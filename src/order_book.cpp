#include "order_book.h"

namespace OrderBook {

SmartOrderBook::SmartOrderBook()
  : sides_{BookSide(false, BidComparator()), BookSide(true, AskComparator())} {
}

/*
 * Process incoming orders on this side
 * If there is pending liq add qty at the order price level, match the pending qty first
 * If there is remaining qty, check whether the order will make the order book crosed
 * If tehre is remaining qty after uncrossing the book, add the order in current side
 */
auto SmartOrderBook::processOrderAddMessage(const OrderMessage& msg) -> OrderInfoVec {
  OrderInfoVec events;
  auto order = msg.toOrder();
  auto matched_qty = sides_[msg.is_sell].matchPendingLiqAdd(msg.quantity, msg.price);
  order->filled_quantity += matched_qty;
  if (order->getRemainingQuantity() == 0) return events;
  // check whether it's crossed
  if (sides_[1-msg.is_sell].bookCrossedWithPrice(msg.price)) {
    auto uncross_events = sides_[1-msg.is_sell].processCrossedOrder(order);
    sides_[msg.is_sell].addPendingLiqRemoveQty(uncross_events);
    mergeEvents(events, uncross_events);
  }
  if (order->getRemainingQuantity() == 0) return events;
  sides_[msg.is_sell].addOrder(order);
  events.emplace_back(OrderEvent::ADD, msg.id, msg.is_sell, order->getRemainingQuantity(), order->price);
  return events;
}

auto SmartOrderBook::processOrderCancelMessage(const OrderMessage &msg)-> OrderInfoVec{
  OrderInfoVec events;
  auto& cur_side = sides_[msg.is_sell];
  mergeEvents(events, cur_side.processOrderCancel(msg.id, msg.quantity, msg.price));
  return events;
}

auto SmartOrderBook::processOrderModifyMessage(const OrderMessage& msg)-> OrderInfoVec {
  OrderInfoVec events{OrderInfo(OrderEvent::MODIF, msg.id, msg.is_sell, msg.quantity, msg.price)};
  if (!existOrder(msg.id)) return events;
  // Simulate order modification with cancellation and new order event
  // There are some uncertain points here. When modifiy partially filled order, not sure whether the quantity received is new remaining qty or new original qty. Here will just assume it's new remaining qty.
  auto& cur_order = getOrderHandler(msg.id).order;
  processOrderCancelMessage({MessageType::CANCEL, msg.id, msg.is_sell, cur_order->getRemainingQuantity(), msg.price});
  processOrderAddMessage({MessageType::ADD, msg.id, msg.is_sell, msg.quantity, msg.price});
  return events;
}

auto SmartOrderBook::processTradeMessage(const TradeMessage &msg) -> OrderInfoVec{
  OrderInfoVec events;
  mergeEvents(events, sides_[0].processTrade(msg.toTrade()));
  mergeEvents(events, sides_[1].processTrade(msg.toTrade()));
  return events;
}

auto SmartOrderBook::processSnapshotMessage(const SnapshotMessage &msg)-> OrderInfoVec{
  OrderInfoVec events;
  mergeEvents(events, sides_[0].processL2Snapshot(msg.bid_levels));
  mergeEvents(events, sides_[1].processL2Snapshot(msg.ask_levels));
  return events;
}



auto SmartOrderBook::getL2Book() -> L2Book {
  L2Book book;
  for(auto& [k, v]: sides_[0]) {
    book.addLevel(false, k, v.quantity);
  }
  for(auto& [k, v]: sides_[1]) {
    book.addLevel(true, k, v.quantity);
  }
  return book;
}


} //namespace OrderBook