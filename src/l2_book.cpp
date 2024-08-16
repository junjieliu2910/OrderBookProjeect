#include "l2_book.h"
#include <utility>
#include <iostream>

namespace OrderBook {

bool L2Book::existLevel(const bool is_sell, const Price price) const {
  if (is_sell) {
    return askBook_.find(price) != askBook_.end();
  } else {
    return bidBook_.find(price) != bidBook_.end();
  }
}

auto L2Book::getL2Level(const bool is_sell, const Price price) -> const L2PriceLevel &{
  if (is_sell) {
    return askBook_[price];
  } else {
    return bidBook_[price];
  }
}


void L2Book::addLevel(const bool is_sell, const Price price, const Quantity quantity){
  if (existLevel(is_sell, price)) {
    std::cerr << "[L2Book]: Trying to add an L2 level that already exist: "
              << quantity << "@" << quantity << std::endl;
    return;
  }
  if (is_sell) {
    askBook_[price] = {price, quantity};
  } else {
    bidBook_[price] = {price, quantity};
  }
}

void L2Book::updateLevel(const bool is_sell, const Price price, const Quantity quantity){
  if (existLevel(is_sell, price)) {
    auto& level = is_sell ? askBook_[price]: bidBook_[price];
    level.price = price;
    level.quantity = quantity;
  }
}

void L2Book::removeLevel(const bool is_sell, const Price price){
  if (existLevel(is_sell, price)) {
    if (is_sell) {
      askBook_.erase(price);
    } else {
      bidBook_.erase(price);
    }
  }
}

std::ostream& operator<<(std::ostream& os, const L2Book& book){
  for (auto iter = book.askBook_.rbegin(); iter != book.askBook_.rend(); ++iter) {
    os << "A " << iter->second;
  }
  for (auto iter = book.bidBook_.begin(); iter != book.bidBook_.end(); ++iter) {
    os << "B " << iter->second;
  }
  return os;
}


} //namespace OrderBook
