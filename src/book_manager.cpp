#include "book_manager.h"
#include <iostream>

namespace OrderBook {

BookManager::BookManager() {
  events_.reserve(1024);
}


void BookManager::processOrderMessage(OrderMessage msg){
  switch (msg.type) {
    case MessageType::ADD:
      mergeEvents(events_, book_.processOrderAddMessage(msg));
      break;
    case MessageType::CANCEL:
      mergeEvents(events_, book_.processOrderCancelMessage(msg));
      break;
    case MessageType::MODIFY:
      mergeEvents(events_, book_.processOrderModifyMessage(msg));
      break;
    default:
      std::cerr << "Invalid order message type" << std::endl;
      break;
  }
}

void BookManager::processSnapshotMessage(SnapshotMessage msg){
  mergeEvents(events_, book_.processSnapshotMessage(msg));
}

void BookManager::processTradeMessage(TradeMessage msg){
  mergeEvents(events_, book_.processTradeMessage(msg));
}

void BookManager::flushEvents() {
  for (const auto& info: events_) {
    switch (info.event) {
      case OrderEvent::ADD:
        onOrderAdd(book_, info);
        break;
      case OrderEvent::CANCEL:
        onOrderCancel(book_, info);
        break;
      case OrderEvent::EXEC:
        onOrderExecution(book_, info);
        break;
      case OrderEvent::MODIF:
        onOrderModify(book_, info);
        break;
      default:
        std::cerr << "Invalid order event type" << std::endl;
        break;
    }
  }
  events_.clear();
}



} // namespace OrderBook