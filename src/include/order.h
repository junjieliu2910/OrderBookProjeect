#pragma once
#include "common.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <iostream>

namespace OrderBook {

struct Order {
  OrderId odid;
  bool is_sell;
  Quantity quantity;
  Price price;
  Quantity filled_quantity;
  explicit Order(OrderId id, bool is_sell, Quantity quantity, Price price)
    : odid(id), is_sell(is_sell), quantity(quantity), price(price), filled_quantity(0) {}

  [[nodiscard]] Quantity getRemainingQuantity() const {
    return quantity - filled_quantity;
  }

  friend std::ostream& operator<<(std::ostream& os, const Order& order);
};

using OrderPtr = std::shared_ptr<Order>;
using OrderList = std::list<OrderPtr>;
using OrderListIter = std::list<OrderPtr>::iterator;

struct OrderHandler {
  OrderPtr order;
  OrderListIter iter;
};

using OrderMap = std::unordered_map<OrderId, OrderHandler>;

} // namespace OrderBook