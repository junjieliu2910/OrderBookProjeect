#include "order.h"
#include <iomanip>

namespace OrderBook {

std::ostream & operator<<(std::ostream &os, const Order &order){
  // Ouput the order with details. The precisiou is fixed to 2
  os << "[" << order.odid << ", "  << order.quantity << "@"
     << std::fixed << std::setprecision(2) << order.price << "]";
  return os;
}

} // namespace OrderBook