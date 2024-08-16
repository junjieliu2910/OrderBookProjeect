#include <gtest/gtest.h>
#include <sstream>
#include "book_side.h"

namespace {
using namespace OrderBook;

class BookSideTest : public ::testing::Test {
protected:
  void SetUp() override {
    /*
     *  Construct with an initial book
     */
    side_.addOrder(std::make_shared<Order>(1, true, 40, 104));
    side_.addOrder(std::make_shared<Order>(2, true, 80, 103));
    side_.addOrder(std::make_shared<Order>(3, true, 60, 102));
    side_.addOrder(std::make_shared<Order>(4, true, 50, 101));
    side_.addOrder(std::make_shared<Order>(5, true, 60, 100));
  }

  std::string getCurL2Book() {
    std::ostringstream os;
    for (auto iter = side_.rbegin(); iter != side_.rend(); ++iter) {
      os << "A " << iter->second.getL2Level();
    }
    return os.str();
  }

  // test with sell side
  BookSide side_{true, AskComparator()};
};

TEST_F(BookSideTest, initializationTest) {
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, addOrderTest) {
  // Add a order with existing order id, won't change the book
  side_.addOrder(std::make_shared<Order>(1, true, 10, 101));
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  side_.addOrder(std::make_shared<Order>(6, true, 10, 101));
  expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 60@101.00\n" // change from 50@101 ==> 60@101
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_TRUE(side_.existOrder(6));
  side_.addOrder(std::make_shared<Order>(7, true, 20, 105));
  expected_book =
    "A L2: 20@105.00\n" // New level 20@105
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 60@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_TRUE(side_.existOrder(7));
}

TEST_F(BookSideTest, removeOrderTest) {
  // remove with a invalid order id. The book should be unchanged
  side_.removeOrder(10);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  side_.removeOrder(1); // remove order 40@104
  expected_book =
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_FALSE(side_.existOrder(1));
}

TEST_F(BookSideTest, modifyOrder) {
  // modify an invalid order, the book should be unchanged
  side_.modifyOrder(10 , 20, 101);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Modify an order without price change
  side_.modifyOrder(1, 80, 104);
  expected_book =
    "A L2: 80@104.00\n" // 40@104 ==> 80@104
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Modify an order with price change
  // 80@103 ==> 60@102
  side_.modifyOrder(2, 60, 102);
  expected_book =
    "A L2: 80@104.00\n"
    "A L2: 120@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, bookCrossedWithPriceTest) {
  EXPECT_TRUE(side_.bookCrossedWithPrice(100));
  EXPECT_TRUE(side_.bookCrossedWithPrice(101));
  EXPECT_TRUE(side_.bookCrossedWithPrice(102));
  EXPECT_FALSE(side_.bookCrossedWithPrice(99));
  EXPECT_FALSE(side_.bookCrossedWithPrice(98));
  EXPECT_FALSE(side_.bookCrossedWithPrice(10));
}

TEST_F(BookSideTest, existLevelTest) {
  EXPECT_TRUE(side_.existLevel(100));
  EXPECT_TRUE(side_.existLevel(104));
  EXPECT_FALSE(side_.existLevel(99));
  EXPECT_FALSE(side_.existLevel(96));
}

TEST_F(BookSideTest, getLevelTest) {
  auto& level = side_.getL3Level(100);
  EXPECT_EQ(level.quantity, 60);
  EXPECT_EQ(level.price, 100);
  EXPECT_EQ(level.num_orders, 1);
}

TEST_F(BookSideTest, orderSteamLeadTest1) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  // Send a buy order 100@102
  // Will get two trade 60@100, 40@101
  auto aggressor = std::make_shared<Order>(6, false, 100, 102);
  auto events = side_.processCrossedOrder(aggressor);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 10@101.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 2);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 60, 100));
  EXPECT_TRUE(events[1] == OrderInfo(OrderEvent::EXEC, 4, true, 40, 101));
  EXPECT_EQ(aggressor->getRemainingQuantity(), 0);
  EXPECT_FALSE(side_.existOrder(5));
  EXPECT_TRUE(side_.existOrder(4));
  auto& order = side_.getOrderHandler(4).order;
  EXPECT_EQ(order->getRemainingQuantity(), 10);
  EXPECT_EQ(order->filled_quantity, 40);
}

TEST_F(BookSideTest, orderSteamLeadTest2) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  // Send a buy order 100@100
  // Will get a trade 60@100 with 40 remaining qty
  auto aggressor = std::make_shared<Order>(6, false, 100, 100);
  auto events = side_.processCrossedOrder(aggressor);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 1);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 60, 100));
  EXPECT_EQ(aggressor->getRemainingQuantity(), 40);
  EXPECT_FALSE(side_.existOrder(5));
}

TEST_F(BookSideTest, orderSteamLeadTest3) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  // Test the price/time priority for order matching
  // Add 2 new orders  20@100, 30@100
  // Then there are 3 order with price 100.  60@100, 20@100, 30@100
  // When an aggressive order 90@100 arrives, 3 trade 60@100, 20@100, 20@100 are expected
  side_.addOrder(std::make_shared<Order>(6, true, 20, 100));
  side_.addOrder(std::make_shared<Order>(7, true, 30, 100));
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 110@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // Receive an aggressive order 90@100
  auto aggressor = std::make_shared<Order>(8, false, 90, 100);
  auto events = side_.processCrossedOrder(aggressor);
  expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 20@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 3);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 60, 100));
  EXPECT_TRUE(events[1] == OrderInfo(OrderEvent::EXEC, 6, true, 20, 100));
  EXPECT_TRUE(events[2] == OrderInfo(OrderEvent::EXEC, 7, true, 10, 100));
  EXPECT_FALSE(side_.existOrder(5));
  EXPECT_FALSE(side_.existOrder(6));
  EXPECT_TRUE(side_.existOrder(7));
  EXPECT_EQ(aggressor->getRemainingQuantity(), 0);
}

TEST_F(BookSideTest, orderSteamLeadTest4) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  // Send a buy order, the initial qty is 100, but filled qty if 80, Remaining qty 20
  // Will have the same effect as an order 20@100
  // Will get two trade 60@100, 40@101
  auto aggressor = std::make_shared<Order>(6, false, 100, 102);
  aggressor->filled_quantity = 80;
  auto events = side_.processCrossedOrder(aggressor);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 40@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 1);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 20, 100));
  EXPECT_EQ(aggressor->getRemainingQuantity(), 0);
  EXPECT_TRUE(side_.existOrder(5));
  auto& order = side_.getOrderHandler(5).order;
  EXPECT_EQ(order->getRemainingQuantity(), 40);
  EXPECT_EQ(order->filled_quantity, 20);
}

TEST_F(BookSideTest, tradeStreamLeadTest1) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  // Get a trade 20@100
  Trade trade{20, 100};
  auto events = side_.processTrade(trade);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 40@100.00\n";
  // Expect order execution event 20@100
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 1);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 20, 100));
}

TEST_F(BookSideTest, tradeSteamLeadTest2) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";  <= Received a trade 20@100
   */
  Trade trade{20, 100};
  auto events = side_.processTrade(trade);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 40@100.00\n";
  // Expect order execution event 20@100
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 1);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::EXEC, 5, true, 20, 100));
}

TEST_F(BookSideTest, tradeSteamLeadTest3) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n" <= Received a trade 20@102
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
   */
  Trade trade{20, 102};
  auto events = side_.processTrade(trade);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 40@102.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 3);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::CANCEL, 5, true, 60, 100));
  EXPECT_TRUE(events[1] == OrderInfo(OrderEvent::CANCEL, 4, true, 50, 101));
  EXPECT_TRUE(events[2] == OrderInfo(OrderEvent::EXEC, 3, true, 20, 102));
  EXPECT_FALSE(side_.existOrder(5));
  EXPECT_FALSE(side_.existOrder(4));
  EXPECT_TRUE(side_.existOrder(3));

  // The order cancel messages that arrive late should not change the order book
  side_.processOrderCancel(4, 50, 101);
  expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 40@102.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  side_.processOrderCancel(5, 60, 100);
  expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 40@102.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, tradeSteamLeadTest4) {
  /* The initial book
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";

                         <== Receive a trade 30@99
   */
  Trade trade{30, 99};
  auto events = side_.processTrade(trade);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n"
    "A L2: 60@100.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
  EXPECT_EQ(events.size(), 2);
  EXPECT_TRUE(events[0] == OrderInfo(OrderEvent::ADD, -1, true, 30, 99));

  // Will have pending liq add qty 30@99. It can be matched with 30@98
  EXPECT_EQ(side_.matchPendingLiqAdd(10, 100), 0);
  EXPECT_EQ(side_.matchPendingLiqAdd(10, 99), 10);
  EXPECT_EQ(side_.matchPendingLiqAdd(10, 98), 10);
  EXPECT_EQ(side_.matchPendingLiqAdd(10, 90), 10);
}

TEST_F(BookSideTest, l2SnapshotLeadReconcileOrderCancelTest) {
  /* Original book
   * A L2: 40@104.00
   * A L2: 80@103.00
   * A L2: 60@102.00
   * A L2: 50@101.00
   * A L2: 60@100.00
   */
  L2SnapshotSide cur = {
    {104, 40},
    {103, 80},
    {102, 60},
    {101, 50},
  };
  side_.processL2Snapshot(cur);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // The receive a trade or order cancel of 60@100. The book should not cahnge
  side_.processOrderCancel(5, 60, 100);
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, l2SnapshotLeadReconCileTradeTest) {
  /* Original book
   * A L2: 40@104.00
   * A L2: 80@103.00
   * A L2: 60@102.00
   * A L2: 50@101.00
   * A L2: 60@100.00
   */
  L2SnapshotSide cur = {
    {104, 40},
    {103, 80},
    {102, 60},
    {101, 50},
  };
  side_.processL2Snapshot(cur);
  std::string expected_book =
    "A L2: 40@104.00\n"
    "A L2: 80@103.00\n"
    "A L2: 60@102.00\n"
    "A L2: 50@101.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);

  // The receive a trade or order cancel of 60@100. The book should not cahnge
  side_.processTrade({60, 100});
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, l2SnapshotLeadOddSnapTest) {
  // Test a strange l2 snapshot
  L2SnapshotSide cur = {
    {130, 30},
    {120, 20},
    {95, 80},
    {90, 40},
  };
  side_.processL2Snapshot(cur);
  std::string expected_book =
    "A L2: 30@130.00\n"
    "A L2: 20@120.00\n"
    "A L2: 80@95.00\n"
    "A L2: 40@90.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

TEST_F(BookSideTest, l2SnapshotLeadMoreOddSnapTest) {
  // Test a strange l2 snapshot
  L2SnapshotSide cur = {
    {105, 20},
    {103, 10},
  };
  side_.processL2Snapshot(cur);
  std::string expected_book =
    "A L2: 20@105.00\n"
    "A L2: 10@103.00\n";
  EXPECT_EQ(getCurL2Book(), expected_book);
}

}

