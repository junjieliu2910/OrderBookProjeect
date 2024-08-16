#pragma once

#include "book_side.h"
#include "l2_book.h"
#include "message.h"
#include <array>
#include <algorithm>
#include <iterator>

namespace OrderBook {

class SmartOrderBook {
public:
  SmartOrderBook();
  ~SmartOrderBook() = default;
  SmartOrderBook(const BookSide& rhs) = delete;
  SmartOrderBook(BookSide&& rhs) = delete;
  SmartOrderBook& operator=(const SmartOrderBook& rhs) = delete;
  SmartOrderBook& operator=(SmartOrderBook&& rhs) = delete;

  auto processOrderAddMessage(const OrderMessage& msg) -> OrderInfoVec;
  auto processOrderCancelMessage(const OrderMessage& msg) -> OrderInfoVec;
  auto processOrderModifyMessage(const OrderMessage& msg) -> OrderInfoVec;
  auto processTradeMessage(const TradeMessage& msg) -> OrderInfoVec;
  auto processSnapshotMessage(const SnapshotMessage& msg) -> OrderInfoVec;

  auto getL2Book() -> L2Book;

  bool existOrder(OrderId id) const {
    return sides_[0].existOrder(id) || sides_[1].existOrder(id);
  }

  auto getOrderHandler(OrderId id) -> const OrderHandler& {
    if (sides_[0].existOrder(id)) return sides_[0].getOrderHandler(id);
    return sides_[1].getOrderHandler(id);
  }

private:
  // sides_[0] is bid side, side[1] is ask side
  std::array<BookSide, 2> sides_;
};

} //namespace OrderBook