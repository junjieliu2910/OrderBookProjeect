#include "level.h"
#include <cassert>
#include <iomanip>

namespace OrderBook {

auto L3PriceLevel::addOrder(const OrderPtr& order) -> OrderListIter {
  quantity += order->getRemainingQuantity();
  price = order->price;
  num_orders += 1;
  orders.push_back(order);
  auto iter = orders.end();
  std::advance(iter, -1);
  return iter;
}

void L3PriceLevel::removeOrder(const OrderHandler &handler){
  orders.erase(handler.iter);
  quantity -= handler.order->getRemainingQuantity();
  num_orders -= 1;
}

void L3PriceLevel::modifyOrder(OrderPtr order, const Quantity new_quantity, const Price new_price){
  assert(new_price == order->price);
  quantity += new_quantity - order->quantity;
  order->quantity = new_quantity;
}

void L3PriceLevel::fillOrder(OrderPtr order, Quantity qty){
  assert(qty <= order->getRemainingQuantity());
  order->filled_quantity += qty;
  quantity -= qty;
}


std::ostream& operator<<(std::ostream& os, const L2PriceLevel& level) {
  os << "L2: " << level.quantity << "@"
     << std::fixed << std::setprecision(2) << level.price << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const L3PriceLevel& level) {
  os << "L3: " << level.quantity << "@"
     << std::fixed << std::setprecision(2) << level.price << std::endl;
  os << "Orders:(";
  for (auto& order: level.orders) {
    os << (*order);
  }
  os << ")" << std::endl;
  return os;
}

} //namespace OrderBook