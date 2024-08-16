#pragma once
#include <map>
#include <vector>
#include "level.h"
#include "trade.h"

namespace OrderBook {

class BookSide {
public:
  BookSide(const bool is_sell, PriceComparator comp);
  ~BookSide() = default;

  BookSide(const BookSide& rhs) = delete;
  BookSide(BookSide&& rhs) = delete;
  BookSide& operator=(const BookSide& rhs) = delete;
  BookSide& operator=(BookSide&& rhs) = delete;

  auto begin() { return levels_.begin(); }
  auto end() { return levels_.end(); }
  auto rbegin() { return levels_.rbegin(); }
  auto rend() { return levels_.rend(); }
  auto cbegin() const { return levels_.cbegin(); }
  auto cend() const { return levels_.cend(); }

  bool existOrder(OrderId id) const {
    return order_map_.find(id) != order_map_.end();
  }

  // Assume the order exist
  auto getOrderHandler(OrderId id) -> const OrderHandler& {
    return order_map_[id];
  }

  /*
   * Add an order to current order book, assume this order won't make the order book crossed and can be added
   */
  void addOrder(const OrderPtr& order);

  /*
   * Remove an order from current order book
   * Will use it to handle order cancellation
   */
  void removeOrder(OrderId id);

  /*
   * Modify the order with new qty and price
   * Assume that the side of the order cannot be changed
   */
  void modifyOrder(OrderId odid, const Quantity quantity, const Price price);

  // Check whether current order book side will be crossed with new order on the other book side
  bool bookCrossedWithPrice(const Price price) const;

  // Check whether an price level exists
  bool existLevel(const Price price) const {
    return levels_.find(price) != levels_.end();
  }

  // Assume the level exist, need to be used with existLevel
  auto getL3Level(const Price price) -> L3PriceLevel& {
    return levels_[price];
  }

  /*
   * Process the opposite aggreive order that will cross with current book side
   * Uncross the order book and add pending_liq_remove_qty_ since incoming trades are expected
   * Also update the quantity of the order
   */
  auto processCrossedOrder(const OrderPtr& order) -> OrderInfoVec;


  /*
   * Need to match with pending liq remove qty
   */
  auto processOrderCancel(OrderId id, const Quantity quantity, const Price price) -> OrderInfoVec;

  /*
   * Process trade message received. Trade is liquidity removing event.
   * Try to match the coming trade in pending_liq_remove_qty_ first.
   * If the trade can be matched with pending qty, the corresponding qty should be reduced from pending qty
   * If there is still remaining qty, try to match with current levels and remove correspnding qty
   * If there is still remaining qty up to now, some liquidity adding events are expected. Then add the qty to pending_liq_add_qty_
   */
  auto processTrade(const Trade& trade) -> OrderInfoVec;

  /*
   * Process the L2 snapshot on this side
   * When liquidity is removed, will guess order cancellation and execution events based on 30% filled ratio
   */
  auto processL2Snapshot(const L2SnapshotSide& side) -> OrderInfoVec;

  /*
   * Matched with the pending liq adding qty and return the matched quantity
   */
  auto matchPendingLiqAdd(const Quantity quantity, const Price price) -> Quantity;

  /*
   * Matched with the pending liq removing qty and return the matched quantity
   */
  auto matchPendingLiqRemove(const Quantity quantity, const Price price) -> Quantity;

  /*
   * Add the pending liq removing qty
   */
  void addPendingLiqRemoveQty(const OrderInfoVec& events);

  void saveL2SnapshoSide();

  friend std::ostream& operator<<(std::ostream& os, const BookSide& side);

private:
  const bool is_sell_;
  const PriceComparator comp_;
  OneSideBook<L3PriceLevel, PriceComparator> levels_;
  OrderMap order_map_;

  /*
   * Will save a L2SnapshotSide when the order book status changes
   */
  L2SnapshotSideQue l2_snap_queue_;

  /* Store the pending qty for liquidity removing events
   * Liquidity remove events includes trades and cancellation. They can be matched with precise price
   */
  std::unordered_map<Price, Quantity> pending_liq_remove_qty_;

  /* Store the pending qty for liquidity adding events
   * Liquidity add events cannot be matched with precise price
   * For example, with following order book
   *   Bid       Ask
   *           30@101
   *           20@100
   *   10@99
   *   20@98
   *
   * When receiving a trade 10@100, it might be triggered by an aggressive order with any price >= 100
   * So a map is used to store the pending qty. The comparator is the same as levels
   * Any incoming order event that can beat the map top will match the top quantity
   */
  std::map<Price, Quantity, PriceComparator> pending_liq_add_qty_{comp_};
};

} //namespace OrderBook