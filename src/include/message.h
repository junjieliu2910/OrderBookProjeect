#pragma once
#include "common.h"
#include "order.h"
#include "level.h"

namespace OrderBook {

enum class MessageType {
  ADD,        // Add new order
  CANCEL,     // Cancel existing orders
  MODIFY,     // Modify existing orders
  EXEC,       // Execution existing orders
  TRADE,      // Trade info
  SNAPSHOT    // L2 book snapshot
};

struct OrderMessage {
  MessageType type;
  OrderId id;
  bool is_sell;
  Quantity quantity;
  Price price;

  [[nodiscard]] OrderPtr toOrder() const {
    return std::make_shared<Order>(id, is_sell, quantity, price);
  }
};

struct TradeMessage {
  Quantity quantity;
  Price price;
  TradeMessage(Quantity q, Price p): quantity(q), price(p) {}
  [[nodiscard]] Trade toTrade() const {
    return{quantity, price};
  }
};

struct SnapshotMessage {
  // Assume bid and ask level in snapshot is already sorted with correct order
  L2SnapshotSide bid_levels;
  L2SnapshotSide ask_levels;
};

} //namespace OrderBook