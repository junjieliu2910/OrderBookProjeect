#include <gtest/gtest.h>
#include <sstream>
#include "order_book.h"

namespace {
using namespace OrderBook;

class SmartOrderBookTest : public ::testing::Test {
protected:
  void SetUp() override {
    /* Construct an initial order book with following status
     * Format [qty@price]
     * Each order will have the same qty 10
     *      Bid         Ask
     *                60@104  (6 x 10@104)
     *                70@103  (7 x 10@103)
     *               110@102  (11 x 10@102)
     *                30@101  (3 x 10@101)
     *     20@95              (2 x 10@95)
     *    130@94              (13 x 10@94)
     *     70@93              (7 x 10@93)
     *     50@92              (5 x 10@92)
     */
    addOrders(6, true, 104);
    addOrders(7, true, 103);
    addOrders(11, true, 102);
    addOrders(3, true, 101);
    addOrders(2, false, 95);
    addOrders(13, false, 94);
    addOrders(7, false, 93);
    addOrders(5, false, 92);
  }

  void addOrders(int nums, bool is_sell, Price price) {
    for (int i = 0; i < nums; ++i) {
      book_.processOrderAddMessage({MessageType::ADD, id++, is_sell, 10, price});
    }
  }

  std::string getCurL2Book() {
    std::ostringstream os;
    os << book_.getL2Book();
    return os.str();
  }

  int id{1};
  SmartOrderBook book_;
  std::string original_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
};

TEST_F(SmartOrderBookTest, initializationTest) {
  EXPECT_EQ(getCurL2Book(), original_book);
}

TEST_F(SmartOrderBookTest, orderAddTest) {
  // Add an order 10@105, won't cross orderbook
  auto events = book_.processOrderAddMessage({MessageType::ADD, 100, true, 10, 105});
  std::string expected_book =
    "A L2: 10@105.00\n"
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 1);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::ADD, 100, true, 10, 105));
}

TEST_F(SmartOrderBookTest, orderSteamLeadTest) {
  // Add an aggressive bid order 90@102
  auto events = book_.processOrderAddMessage({MessageType::ADD, 100, false, 90, 102});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 50@102.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 9);
  // Receive the expected trades. Won't change order book
  for (int i = 0; i < 3; ++i) {
    book_.processTradeMessage({10, 101});
  }
  EXPECT_EQ(getCurL2Book(), expected_book);
  for (int i = 0; i < 6; ++i) {
    book_.processTradeMessage({10, 102});
  }
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, orderSteamLeadTest2) {
  // Add an aggressive bid order 50@101
  auto events = book_.processOrderAddMessage({MessageType::ADD, 100, false, 50, 101});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "B L2: 20@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 4);
  // Receive the expected trades. Won't change order book
  for (int i = 0; i < 3; ++i) {
    book_.processTradeMessage({10, 101});
  }
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive an unexpected tarde. Will change the order book
  book_.processTradeMessage({10, 101});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "B L2: 10@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}


TEST_F(SmartOrderBookTest, orderSteamLeadTest3) {
  // Add an aggressive ask order 50@101
  auto events = book_.processOrderAddMessage({MessageType::ADD, 100, true, 40, 95});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "A L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 3);

  // Receive the expected trades. Won't change order book
  for (int i = 0; i < 2; ++i) {
    book_.processTradeMessage({10, 95});
  }
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive an unexpected trade. Will change the order book
  book_.processTradeMessage({10, 95});
  EXPECT_TRUE(getCurL2Book() != expected_book);
}

TEST_F(SmartOrderBookTest, tradeSteamLeadTest1) {
  // Receive two trade 10@101, 10@95 first
  auto events = book_.processTradeMessage({10, 101});
  auto events2 = book_.processTradeMessage({10, 95});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 20@101.00\n"  // ==> change from 30 to 20
    "B L2: 10@95.00\n"   // ==> change from 20 to 10
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Received the expect orders. although the orders are aggressive but they won't change the order book
  book_.processOrderAddMessage({MessageType::ADD, 100, false, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderAddMessage({MessageType::ADD, 101,true, 10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive another new order. Will change order book
  book_.processOrderAddMessage({MessageType::ADD, 102, false, 10, 100});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 20@101.00\n"
    "B L2: 10@100.00\n"
    "B L2: 10@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, tradeSteamLeadTest2) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)   <== Received a trade 10@102
  *                30@101  (3 x 10@101)
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94)
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Receive a trade 10@102 first
  book_.processTradeMessage({10, 102});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 100@102.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the order cancellation later
  book_.processOrderCancelMessage({MessageType::CANCEL, 25, true, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderCancelMessage({MessageType::CANCEL, 26, true, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderCancelMessage({MessageType::CANCEL, 27, true, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the liquidity taking order on bid side 10@102, although it's aggressive order, but won't change the order book
  book_.processOrderAddMessage({MessageType::ADD, 100, false, 10, 102});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the same order, but this time the order book will change
  book_.processOrderAddMessage({MessageType::ADD, 101, false, 10, 102});
  EXPECT_TRUE(getCurL2Book() != expected_book);
}

TEST_F(SmartOrderBookTest, tradeSteamLeadTest3) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)
  *                30@101  (3 x 10@101)
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94) <== Received a trade 10@94
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Receive a trade 10@102 first
  book_.processTradeMessage({10, 94});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 120@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the order cancellation later
  book_.processOrderCancelMessage({MessageType::CANCEL, 28, false, 10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderCancelMessage({MessageType::CANCEL, 29, false, 10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the liquidity taking order on ask side 10@94, although it's aggressive order, but won't change the order book
  book_.processOrderAddMessage({MessageType::ADD, 100, true, 10, 94});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the same order, but this time the order book will change
  book_.processOrderAddMessage({MessageType::ADD, 101, true, 10, 94});
  EXPECT_TRUE(getCurL2Book() != expected_book);
}

TEST_F(SmartOrderBookTest, tradeSteamLeadTest4) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)
  *                30@101  (3 x 10@101)
  *                                      <== Received a trade 10@99
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94)
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Receive a trade 10@99, the order book keeps unchanged
  book_.processTradeMessage({10, 99});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive two expected orders Ask 10@99 and Bid 10@99, book keeps unchanged
  book_.processOrderAddMessage({MessageType::ADD, 100, false, 10, 99});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderAddMessage({MessageType::ADD, 101, true, 10, 99});
  EXPECT_EQ(getCurL2Book(), expected_book);

  // From now on, the incoming new orders will change the order book
  // Two same orders Ask 10@99 and Bid 10@99 received. The order book will change
  book_.processOrderAddMessage({MessageType::ADD, 102, false, 10, 98});
  book_.processOrderAddMessage({MessageType::ADD, 103, true, 10, 99});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "A L2: 10@99.00\n"
    "B L2: 10@98.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, testNormalOrderModif) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)
  *                30@101  (3 x 10@101)
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94)
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Modif the order from Ask 10@104 to Ask 20@104
  book_.processOrderModifyMessage({MessageType::MODIFY, 1, true, 20, 104});
  std::string expected_book =
    "A L2: 70@104.00\n"  // ==> changed from 60 to 70 here
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, testOrderModifAndOrderLeadTrade) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)
  *                30@101  (3 x 10@101)
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94)
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Modify the order from Ask 10@104 to Ask 10@94, will trigger a trade 10@95
  book_.processOrderModifyMessage({MessageType::MODIFY, 1, true, 10, 95});
  std::string expected_book =
    "A L2: 50@104.00\n"  // ==> change from 60 to 50 here
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 10@95.00\n"  // ==> change 20 to 10 here
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive the trade, the book should keep unchanged
  book_.processTradeMessage({10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, testOrderModifAndTradeLeadOrder) {
  /*     Bid         Ask
  *                60@104  (6 x 10@104)
  *                70@103  (7 x 10@103)
  *               110@102  (11 x 10@102)
  *                30@101  (3 x 10@101)
  *                                      <== Received a trade 10@99
  *     20@95              (2 x 10@95)
  *    130@94              (13 x 10@94)
  *     70@93              (7 x 10@93)
  *     50@92              (5 x 10@92)
  */
  // Receive a trade 10@99
  book_.processTradeMessage({10, 99});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 20@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive a order modif from 10@95 => 10@99,
  book_.processOrderModifyMessage({MessageType::MODIFY, 28, false, 10, 99});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 30@101.00\n"
    "B L2: 10@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive a order modif from 10@101 => 10@99,
  book_.processOrderModifyMessage({MessageType::MODIFY, 27, true, 10, 99});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 20@101.00\n"
    "B L2: 10@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive a order B 10@99, this order will show in order book
  book_.processOrderAddMessage({MessageType::ADD, 100, false, 10, 99});
  expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 20@101.00\n"
    "B L2: 10@99.00\n"
    "B L2: 10@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, testL2SnapLeadLiqRemove) {
  /* Construct an initial order book with following status
   * Format [qty@price]
   * Each order will have the same qty 10
   *      Bid         Ask
   *                60@104  (6 x 10@104)
   *                70@103  (7 x 10@103)
   *               110@102  (11 x 10@102)
   *                30@101  (3 x 10@101)
   *     20@95              (2 x 10@95)
   *    130@94              (13 x 10@94)
   *     70@93              (7 x 10@93)
   *     50@92              (5 x 10@92)
   */
  L2SnapshotSide ask = {
    {104, 60},
    {103, 70},
    {102, 110},
    {101, 20},
  };
  L2SnapshotSide bid = {
    {95, 10},
    {94, 130},
    {93, 70},
    {92, 50},
  };
  auto events = book_.processSnapshotMessage({bid, ask});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 20@101.00\n"
    "B L2: 10@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive two cancel. But it should not udpate the orderbook
  book_.processOrderCancelMessage({MessageType::CANCEL, 25, true, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderCancelMessage({MessageType::CANCEL, 28, false, 10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(SmartOrderBookTest, testL2SnapLeadLiqAdd) {
  /* Construct an initial order book with following status
   * Format [qty@price]
   * Each order will have the same qty 10
   *      Bid         Ask
   *                60@104  (6 x 10@104)
   *                70@103  (7 x 10@103)
   *               110@102  (11 x 10@102)
   *                30@101  (3 x 10@101)
   *     20@95              (2 x 10@95)
   *    130@94              (13 x 10@94)
   *     70@93              (7 x 10@93)
   *     50@92              (5 x 10@92)
   */
  L2SnapshotSide ask = {
    {104, 60},
    {103, 70},
    {102, 110},
    {101, 40},  // ==> from 30 to 40
  };
  L2SnapshotSide bid = {
    {95, 30}, // ==> from 20 to 30
    {94, 130},
    {93, 70},
    {92, 50},
  };
  auto events = book_.processSnapshotMessage({bid, ask});
  std::string expected_book =
    "A L2: 60@104.00\n"
    "A L2: 70@103.00\n"
    "A L2: 110@102.00\n"
    "A L2: 40@101.00\n"
    "B L2: 30@95.00\n"
    "B L2: 130@94.00\n"
    "B L2: 70@93.00\n"
    "B L2: 50@92.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive two cancel. But it should not udpate the orderbook
  book_.processOrderAddMessage({MessageType::ADD, 100, true, 10, 101});
  EXPECT_EQ(getCurL2Book(), expected_book);
  book_.processOrderAddMessage({MessageType::ADD, 101, false, 10, 95});
  EXPECT_EQ(getCurL2Book(), expected_book);
}

}
