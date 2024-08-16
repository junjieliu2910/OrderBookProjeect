#pragma once
#include <deque>
#include "common.h"
#include "order.h"

namespace OrderBook {

struct L2PriceLevel {
  Price price;
  Quantity quantity;
  L2PriceLevel(): price(0), quantity(0) {}
  L2PriceLevel(Price p, Quantity q): price(p), quantity(q) {}
  bool operator==(const L2PriceLevel& rhs) const {
    return price == rhs.price && quantity == rhs.quantity;
  }

  friend std::ostream& operator<<(std::ostream& os, const L2PriceLevel& level);
};

using L2SnapshotSide = std::vector<L2PriceLevel>;
using L2SnapshotSideQue = std::deque<L2SnapshotSide>;


struct L3PriceLevel {
  Price price;
  Quantity quantity;
  int num_orders;
  OrderList orders;

  L3PriceLevel(): price(0), quantity(0), num_orders(0), orders() {}

  // Assume that this order can be added to this limit. The sanity check should be done at book level
  auto addOrder(const OrderPtr& order) -> OrderListIter;

  // Assume that the order exist in the order list
  void removeOrder(const OrderHandler& handler);

  /*
   * Assume the order exist, the existence check should be done at book level
   * and the order modification only modify the quantity
   * The order modification that also changes price will be handled differently
   * Only modify the original quantity
   */
  void modifyOrder(OrderPtr order, const Quantity new_quantity, const Price new_price);

  // Fill the order with quantity, assume that the remaining qty is larger than the quantity to be filed
  void fillOrder(OrderPtr order, Quantity qty);

  // Get the L2 level
  auto getL2Level() const -> L2PriceLevel {
    return {price, quantity};
  }


  friend std::ostream& operator<<(std::ostream& os, const L3PriceLevel& level);
};

} // namespace OrderBook