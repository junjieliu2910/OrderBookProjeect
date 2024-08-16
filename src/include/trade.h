#pragma once
#include "common.h"

namespace OrderBook {

struct Trade {
  Quantity quantity;
  Price price;
  Trade(Quantity qty, Price price) : quantity(qty), price(price) {}
};

} // namespace OrderBook