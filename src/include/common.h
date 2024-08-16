#pragma once
#include <common.h>
#include <functional>
#include <map>

namespace OrderBook {

using Quantity = int;
using Price = double;
using OrderId = int;
using PriceComparator = std::function<bool(const Price&, const Price&)>;

struct BidComparator {
  bool operator()(const Price& lhs, const Price& rhs) const {
    return lhs > rhs;
  }
};

struct AskComparator {
  bool operator()(const Price& lhs, const Price& rhs) const {
    return lhs < rhs;
  }
};

//template <typename LevelType, typename Comparator>
//using BookItermAllocator = PoolAllocator<typename std::map<Price, LevelType, Comparator>::value_type>;
//
//template <typename LevelType, typename Comparator>
//using OneSideBook = std::map<Price, LevelType, Comparator, BookItermAllocator<LevelType, Comparator>>;

template <typename LevelType, typename Comparator>
using OneSideBook = std::map<Price, LevelType, Comparator>;

enum class OrderEvent {
  ADD,
  CANCEL,
  EXEC,
  MODIF
};

struct OrderInfo {
  OrderEvent event;
  OrderId odid;
  bool is_sell;
  Quantity quantity;
  Price price;
  OrderInfo(OrderEvent e, OrderId id, bool sell, Quantity qty, Price p) : event(e), odid(id), is_sell(sell), quantity(qty), price(p) {}
  bool operator==(const OrderInfo& rhs) const {
    return event == rhs.event && odid == rhs.odid && quantity == rhs.quantity && price == rhs.price;
  }
};

using OrderInfoVec = std::vector<OrderInfo>;

// Merge events of b into a
inline void mergeEvents(OrderInfoVec& a, OrderInfoVec b) {
  a.reserve(a.size() + b.size());
  std::move(b.begin(), b.end(), std::back_inserter(a));
}


} // namespace OrderBook