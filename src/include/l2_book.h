#pragma once
#include "level.h"
#include "trade.h"

namespace OrderBook {


class L2Book {
public:
  explicit L2Book() = default;
  L2Book(OneSideBook<L2PriceLevel, BidComparator> bids, OneSideBook<L2PriceLevel, AskComparator> asks)
    : bidBook_(std::move(bids)), askBook_(std::move(asks)) {}
  ~L2Book() = default;
  L2Book(const L2Book& rhs) = default;
  L2Book(L2Book&& rhs) = default;
  L2Book& operator=(const L2Book& rhs) = default;
  L2Book& operator=(L2Book&& rhs) = default;


  bool existLevel(const bool is_sell, const Price price) const;
  // Assume the level exist, need to be used with existLevel
  auto getL2Level(const bool is_sell, const Price price) -> const L2PriceLevel&;
  void addLevel(const bool is_sell, const Price price, const Quantity quantity);
  void updateLevel(const bool is_sell, const Price price, const Quantity quantity);
  void removeLevel(const bool is_sell, const Price price);
  friend std::ostream& operator<<(std::ostream& os, const L2Book& book);

private:
  OneSideBook<L2PriceLevel, BidComparator> bidBook_;
  OneSideBook<L2PriceLevel, AskComparator> askBook_;
};



} // namesapce OrderBook