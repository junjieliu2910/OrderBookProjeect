#include "book_side.h"
#include <cassert>
#include <cmath>
#include <unordered_set>

namespace OrderBook {

BookSide::BookSide(const bool is_sell, PriceComparator comp)
  : is_sell_(is_sell), comp_(std::move(comp)), levels_(comp_) {
  order_map_.reserve(1024);
  pending_liq_remove_qty_.reserve(32);
}

void BookSide::addOrder(const OrderPtr& order) {
  assert(order->is_sell == is_sell_);
  if (existOrder(order->odid)) return;
  auto iter = levels_[order->price].addOrder(order);
  order_map_[order->odid] = {order, iter};
}

void BookSide::removeOrder(OrderId id){
  if (!existOrder(id)) return;
  auto& handler = order_map_[id];
  auto& cur_level = levels_[handler.order->price];
  cur_level.removeOrder(handler);
  order_map_.erase(id);
  if (cur_level.num_orders == 0) {
    levels_.erase(cur_level.price);
  }
}

void BookSide::modifyOrder(OrderId odid, const Quantity quantity, const Price price){
  if (!existOrder(odid)) {
    return;
  }
  auto& order = order_map_[odid].order;
  if (order->price == price) {
    // Modify order without price change
    auto& level = getL3Level(price);
    level.modifyOrder(order, quantity, price);
  } else {
    // Modify order with price change, remove the old order and add a new order
    auto new_order = std::make_shared<Order>(order->odid, order->is_sell, quantity, price);
    new_order->filled_quantity = order->filled_quantity;
    removeOrder(odid);
    addOrder(new_order);
  }
}

bool BookSide::bookCrossedWithPrice(const Price price) const {
  if (levels_.empty()) return false;
  return is_sell_ ? price >= levels_.begin()->second.price : levels_.begin()->second.price >= price;
}

/*
 * With the assumption that messages in order steam come in order
 * Then book crosses, the crossed orders are expected to be filled
 * No need to guess here.
 *
 * The code logic can also handle the case that order meesages arrive out of order
 * only need to add the guess logic
 */
auto BookSide::processCrossedOrder(const OrderPtr &order) -> OrderInfoVec {
  // Need to pass the aggressor
  assert(order->is_sell != is_sell_);
  Quantity remaining_quantity = order->getRemainingQuantity();
  OrderInfoVec order_events;
  while (remaining_quantity > 0 && bookCrossedWithPrice(order->price)) {
    auto& cur_level = levels_.begin()->second;
    std::vector<OrderId> orders_to_remove;
    for (auto& cur_order: cur_level.orders) {
      if (remaining_quantity == 0) break;
      Quantity fillable_qty = std::min(remaining_quantity, cur_order->getRemainingQuantity());
      cur_level.fillOrder(cur_order, fillable_qty);
      saveL2SnapshoSide();
      remaining_quantity -= fillable_qty;
      if (cur_order->getRemainingQuantity() == 0) {
        orders_to_remove.push_back(cur_order->odid);
      }
      order_events.emplace_back(OrderEvent::EXEC, cur_order->odid, is_sell_, fillable_qty, cur_order->price);
      // Expect trade messages will be received
      // Trade is liquidity remove event so incease pending liq remove qty
      pending_liq_remove_qty_[cur_order->price] += fillable_qty;
      order->filled_quantity += fillable_qty;
    }
    for (auto odid: orders_to_remove) {
      removeOrder(odid);
    }
  }
  return order_events;
}

auto BookSide::processOrderCancel(OrderId id, const Quantity quantity, const Price price) -> OrderInfoVec {
  OrderInfoVec order_events;
  Quantity rest_qty = quantity - matchPendingLiqRemove(quantity, price);
  // If still have qty to cancel then should cancel order in order book
  if (rest_qty > 0 && existOrder(id)) {
    order_events.emplace_back(OrderEvent::CANCEL, id, is_sell_, rest_qty, price);
    removeOrder(id);
  }
  return order_events;
}


auto BookSide::processTrade(const Trade& trade) -> OrderInfoVec {
  // If there is still pending liq remote qty for this trade price
  auto cur_trade_qty = trade.quantity;
  cur_trade_qty -= matchPendingLiqRemove(trade.quantity, trade.price);

  /* If still have trade qty to match, try to match the qty in current limits and update the order book
   * Considering an ask side
   *   Ask
   *  50@102
   *  40@101  <==  Received a trade 10@101
   *  30@100
   *
   *  Steps to handle it
   *  1. Cancel all orders below the price level (above the price leve for bid side)
   *  2. Match the trade with corresponding level
   *  3. Add pending liq add qty for remaining trade qty
   */

  OrderInfoVec order_events;
  // Step 1
  while(!levels_.empty()) {
    std::vector<OrderId> orders_to_remove;
    auto iter = levels_.begin();
    bool should_cancel = is_sell_ ? iter->first < trade.price : iter->first > trade.price;
    if (!should_cancel) break;
    for (auto& order: iter->second.orders) {
      orders_to_remove.push_back(order->odid);
      order_events.emplace_back(OrderEvent::CANCEL, order->odid, is_sell_, order->quantity, order->price);
    }
    for (auto odid: orders_to_remove) {
      removeOrder(odid);
      saveL2SnapshoSide();
    }
  }

  // Step 2
  if (existLevel(trade.price)) {
    auto& cur_level = getL3Level(trade.price);
    std::vector<OrderId> orders_to_remove;
    for (auto& cur_order: cur_level.orders) {
      if (cur_trade_qty == 0) break;
      Quantity fillable_qty = std::min(cur_trade_qty, cur_order->getRemainingQuantity());
      cur_level.fillOrder(cur_order, fillable_qty);
      saveL2SnapshoSide();
      cur_trade_qty -= fillable_qty;
      if (cur_order->getRemainingQuantity() == 0) {
        orders_to_remove.push_back(cur_order->odid);
      }
      order_events.emplace_back(OrderEvent::EXEC, cur_order->odid, is_sell_, fillable_qty, cur_order->price);
    }
    for (auto odid: orders_to_remove) {
      removeOrder(odid);
      saveL2SnapshoSide();
    }
  }

  // Step 3
  if (cur_trade_qty > 0) {
    pending_liq_add_qty_[trade.price] += cur_trade_qty;
    // Guess that there will be a incoming new order,
    // since the odid is assigned by exchange so we are not sure what's the order id, will just use -1
    order_events.emplace_back(OrderEvent::ADD, -1, is_sell_, cur_trade_qty, trade.price);
    order_events.emplace_back(OrderEvent::EXEC, -1, is_sell_, cur_trade_qty, trade.price);
  }
  return order_events;
}

auto BookSide::processL2Snapshot(const L2SnapshotSide& side) -> OrderInfoVec {
  OrderInfoVec order_events;
  if (!l2_snap_queue_.empty() && l2_snap_queue_.front() == side) {
    // received the expected l2 snapshot
    l2_snap_queue_.pop_front();
    return order_events;
  }

  // Use this to generate fake order id
  int fake_order_id = 100;
  if (l2_snap_queue_.empty()) {
    // L2 lead the order and trade steam, then the l2_snap_queue_ should be empty
    // No need to save l2 snapshot in this case
    std::vector<std::pair<OrderPtr, Quantity>> pending_orders;
    pending_orders.reserve(64);
    std::unordered_set<Price> l2_price_set;
    for (const auto& l2_level: side) {
      l2_price_set.insert(l2_level.price);
      if (existLevel(l2_level.price)) {
        if (l2_level.quantity < levels_[l2_level.price].quantity) {
          // Expect liquidity removing events
          // Remove order from the front of list to match the qty
          Quantity qty_to_remove = levels_[l2_level.price].quantity - l2_level.quantity;
          for (auto& order: levels_[l2_level.price].orders) {
            if (qty_to_remove == 0) break;
            Quantity cur_remove_quantity = std::min(qty_to_remove, order->getRemainingQuantity());
            qty_to_remove -= cur_remove_quantity;
            pending_orders.emplace_back(order, cur_remove_quantity);
          }
        } else if (l2_level.quantity > levels_[l2_level.price].quantity) {
          // Expect liquidity adding events
          Quantity cur_qty = l2_level.quantity - levels_[l2_level.price].quantity;
          order_events.emplace_back(OrderEvent::ADD, -1, is_sell_, cur_qty, l2_level.price);
          pending_liq_add_qty_[l2_level.price] += cur_qty;
          addOrder(std::make_shared<Order>(fake_order_id++, is_sell_, cur_qty, l2_level.price));
        }
      } else {
        // Expect liquidity adding events
        // Simply use one large order. Can improve here
        order_events.emplace_back(OrderEvent::ADD, -1, is_sell_, l2_level.quantity, l2_level.price);
        pending_liq_add_qty_[l2_level.price] += l2_level.quantity;
        addOrder(std::make_shared<Order>(fake_order_id++, is_sell_, l2_level.quantity, l2_level.price));
      }
    }

    for (const auto& [price, level]: levels_) {
      // Check the level that is in current book but not in l2 snapshot
      if (l2_price_set.find(price) == l2_price_set.end()) {
        // Expect liquidity remove events
        for (auto& order: level.orders) {
          pending_orders.emplace_back(order, order->getRemainingQuantity());
        }
      }
    }

    // 30% of liqidity removing events will be order execution, the reset will be order cancellation
    // The top 30% of the pending order sorted by descending order on ask side and ascending order in bid side
    auto executed_order_num = ceil(0.3 * pending_orders.size());
    for (size_t i = 0; i < pending_orders.size(); ++i) {
      auto& [order, qty] = pending_orders[i];
      if (i < executed_order_num) {
        order_events.emplace_back(OrderEvent::EXEC, order->odid, is_sell_, qty, order->price);
      } else {
        order_events.emplace_back(OrderEvent::CANCEL, order->odid, is_sell_, qty, order->price);
      }
      if (order->getRemainingQuantity() == qty) {
        removeOrder(order->odid);
      } else {
        if (existLevel(order->price)) {
          levels_[order->price].fillOrder(order, qty);
        }
      }
    }
    return order_events;
  }
  // Error case, receive an polluted snapshot
  return {};
}

auto BookSide::matchPendingLiqAdd(const Quantity quantity, const Price price)-> Quantity{
  Quantity matched_qty = 0;
  while (!pending_liq_add_qty_.empty()) {
    auto iter = pending_liq_add_qty_.begin();
    bool can_match = is_sell_ ?  price <= iter->first: price >= iter->first;
    if (!can_match) break;
    Quantity cur_matched_qty = std::min(iter->second, quantity - matched_qty);
    matched_qty += cur_matched_qty;
    pending_liq_add_qty_[iter->first] -= cur_matched_qty;
    if (pending_liq_add_qty_[iter->first] == 0) {
      pending_liq_add_qty_.erase(price);
    }
    if (matched_qty == quantity) break;
  }
  return matched_qty;
}

auto BookSide::matchPendingLiqRemove(const Quantity quantity, const Price price)-> Quantity{
  Quantity matched_qty = 0;
  if (pending_liq_remove_qty_.find(price) != pending_liq_remove_qty_.end()) {
    matched_qty = std::min(pending_liq_remove_qty_[price], quantity);
    pending_liq_remove_qty_[price] -= matched_qty;
    if (pending_liq_remove_qty_[price] == 0) {
      pending_liq_remove_qty_.erase(price);
    }
  }
  return matched_qty;
}

void BookSide::addPendingLiqRemoveQty(const OrderInfoVec &events){
  for (const auto& e: events) {
    if (e.event == OrderEvent::EXEC) {
      pending_liq_remove_qty_[e.price] += e.quantity;
    }
  }
}

void BookSide::saveL2SnapshoSide() {
  L2SnapshotSide l2_side;
  for (auto& [price, level]: levels_) {
    l2_side.emplace_back(price, level.quantity);
  }
  l2_snap_queue_.push_back(std::move(l2_side));
}


std::ostream& operator<<(std::ostream& os, const BookSide& side) {
  if (side.is_sell_) {
    for (auto iter = side.levels_.rbegin(); iter != side.levels_.rend(); ++iter) {
      os << "A " << iter->second;
    }
  } else {
    for (auto iter = side.levels_.begin(); iter != side.levels_.end(); ++iter) {
      os << "B " << iter->second;
    }
  }
  return os;
}

} //namespace OrderBook