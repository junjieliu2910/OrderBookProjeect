#pragma once
#include "order_book.h"
#include "l2_book.h"
#include "message.h"
#include <map>
#include <cstdint>

namespace OrderBook {

class BookManager {
public:
  explicit BookManager();
  ~BookManager() = default;

  void processOrderMessage(OrderMessage msg);
  void processTradeMessage(TradeMessage msg);
  void processSnapshotMessage(SnapshotMessage msg);

  void flushEvents();

  void onOrderAdd(SmartOrderBook& book, const OrderInfo& info);
  void onOrderCancel(SmartOrderBook& book, const OrderInfo& info);
  void onOrderModify(SmartOrderBook& book, const OrderInfo& info);
  void onOrderExecution(SmartOrderBook& book, const OrderInfo& info);

private:
  SmartOrderBook book_;  // The uncrossed L3 book
  std::vector<OrderInfo> events_;
};

} //namespace OrderBook