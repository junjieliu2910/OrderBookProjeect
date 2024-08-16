#include <gtest/gtest.h>
#include "l2_book.h"
#include <string>
#include <sstream>

namespace {
using namespace OrderBook;

class L2BookTest : public ::testing::Test {
protected:
  void SetUp() override {
    // build a L2 book with following structure
    // bid qty, bid price, ask price, ask qty
    //                      101,  40
    //                      100,  30
    //   20        99
    //   50        98
    book_.addLevel(true, 101, 40);
    book_.addLevel(true, 100, 30);
    book_.addLevel(false, 99, 20);
    book_.addLevel(false, 98, 50);
  };

  std::string getCurL2Book() {
    std::ostringstream os;
    os << book_;
    return os.str();
  }

  L2Book book_;
};

TEST_F(L2BookTest, existLevelTest) {
  EXPECT_TRUE(book_.existLevel(true, 101));
  EXPECT_TRUE(book_.existLevel(true, 100));
  EXPECT_TRUE(book_.existLevel(false, 99));
  EXPECT_TRUE(book_.existLevel(false, 98));
  EXPECT_FALSE(book_.existLevel(true, 98));
  EXPECT_FALSE(book_.existLevel(true, 95));
  EXPECT_FALSE(book_.existLevel(true, 110));
}

TEST_F(L2BookTest, getL2LevelTest) {
  const auto& level = book_.getL2Level(true, 100);
  EXPECT_EQ(level.price, 100);
  EXPECT_EQ(level.quantity, 30);
}

TEST_F(L2BookTest, addLevelTest) {
  // Add a already existing level. Should not change the book
  book_.addLevel(true, 100, 20);
  std::string cur_book = getCurL2Book();
  std::string expected_book =
    "A L2: 40@101.00\n"
    "A L2: 30@100.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n";
  EXPECT_EQ(cur_book, expected_book);
}

TEST_F(L2BookTest, addLevelTest2) {
  book_.addLevel(true, 102, 50);
  book_.addLevel(false, 97.23, 60);
  std::string cur_book = getCurL2Book();
  std::string expected_book =
    "A L2: 50@102.00\n"
    "A L2: 40@101.00\n"
    "A L2: 30@100.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n"
    "B L2: 60@97.23\n";
  EXPECT_EQ(cur_book, expected_book);
}

TEST_F(L2BookTest, updateLevelTest) {
  // Update a level that don't exists. The book should be unchanged
  book_.updateLevel(true, 102, 50);
  std::string cur_book = getCurL2Book();
  std::string expected_book =
    "A L2: 40@101.00\n"
    "A L2: 30@100.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n";
  EXPECT_EQ(cur_book, expected_book);

  book_.updateLevel(true, 101, 80);
  cur_book = getCurL2Book();
  expected_book =
    "A L2: 80@101.00\n"
    "A L2: 30@100.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n";
  EXPECT_EQ(cur_book, expected_book);
}

TEST_F(L2BookTest, removeLevelTest) {
  // Remove a level that don't exist. The book should be unchanged
  book_.removeLevel(true, 103);
  std::string cur_book = getCurL2Book();
  std::string expected_book =
    "A L2: 40@101.00\n"
    "A L2: 30@100.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n";
  EXPECT_EQ(cur_book, expected_book);

  // Remove A L2: 30@100
  book_.removeLevel(true, 100);
  cur_book = getCurL2Book();
  expected_book =
    "A L2: 40@101.00\n"
    "B L2: 20@99.00\n"
    "B L2: 50@98.00\n";
  EXPECT_EQ(cur_book, expected_book);

  book_.removeLevel(false, 98);
  cur_book = getCurL2Book();
  expected_book =
    "A L2: 40@101.00\n"
    "B L2: 20@99.00\n";
  EXPECT_EQ(cur_book, expected_book);
}

TEST_F(L2BookTest, addUpdateAndRemoveTest) {
  book_.addLevel(false, 97, 40);
  book_.addLevel(true, 102, 40);
  book_.updateLevel(true, 101, 10);
  book_.updateLevel(true, 100, 30);
  book_.updateLevel(false, 99, 90);
  book_.removeLevel(false, 98);
  book_.removeLevel(true,100);
  book_.removeLevel(true,102);
  std::string cur_book = getCurL2Book();
  std::string expected_book =
    "A L2: 10@101.00\n"
    "B L2: 90@99.00\n"
    "B L2: 40@97.00\n";
  EXPECT_EQ(cur_book, expected_book);
}
}