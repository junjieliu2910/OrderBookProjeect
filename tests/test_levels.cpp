#include <gtest/gtest.h>
#include "level.h"

namespace {
using namespace OrderBook;

TEST(L2levelTest, initializationTest) {
  L2PriceLevel l{2.3, 100};
  EXPECT_EQ(l.price, 2.3);
  EXPECT_EQ(l.quantity, 100);
}

TEST(L2levelTest, updateTest) {
  L2PriceLevel l{2.3, 100};
  EXPECT_EQ(l.price, 2.3);
  EXPECT_EQ(l.quantity, 100);
  l.quantity = 50;
  EXPECT_EQ(l.price, 2.3);
  EXPECT_EQ(l.quantity, 50);
}

TEST(L3levelTest, initializationTest) {
  L3PriceLevel l3;
  EXPECT_EQ(l3.price, 0);
  EXPECT_EQ(l3.quantity, 0);
  EXPECT_EQ(l3.num_orders, 0);
  EXPECT_TRUE(l3.orders.empty());
}

TEST(L3levelTest, addOrderTest) {
  L3PriceLevel l3;
  auto order = std::make_shared<Order>(1, true, 100, 101);
  auto iter = l3.addOrder(order);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 100);
  auto cur_order = *iter;
  EXPECT_EQ(cur_order->is_sell, true);
  EXPECT_EQ(cur_order->quantity, 100);
  EXPECT_EQ(cur_order->price, 101);
}

TEST(L3levelTest, removeOrderTest) {
  L3PriceLevel l3;
  // Add the first order
  auto order = std::make_shared<Order>(1, true, 100, 101);
  auto iter = l3.addOrder(order);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 100);
  auto cur_order = *iter;
  EXPECT_EQ(cur_order->is_sell, true);
  EXPECT_EQ(cur_order->quantity, 100);
  EXPECT_EQ(cur_order->price, 101);

  // Add the second order
  auto another_order = std::make_shared<Order>(2, true, 300, 101);
  auto another_iter = l3.addOrder(another_order);
  EXPECT_EQ(l3.num_orders, 2);
  EXPECT_EQ(l3.quantity, 400);
  cur_order = *another_iter;
  EXPECT_EQ(cur_order->is_sell, true);
  EXPECT_EQ(cur_order->quantity, 300);
  EXPECT_EQ(cur_order->price, 101);

  // Remove the first order
  OrderHandler handler = {order, iter};
  l3.removeOrder(handler);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 300);
  cur_order = *l3.orders.begin();
  EXPECT_EQ(cur_order->is_sell, true);
  EXPECT_EQ(cur_order->quantity, 300);
  EXPECT_EQ(cur_order->price, 101);

  // Remove the second order
  handler = {another_order, another_iter};
  l3.removeOrder(handler);
  EXPECT_EQ(l3.num_orders, 0);
  EXPECT_EQ(l3.quantity, 0);
  EXPECT_TRUE(l3.orders.empty());
}

TEST(L3levelTest, modifyOrderTest) {
  L3PriceLevel l3;
  auto order = std::make_shared<Order>(1, true, 100, 101);
  auto iter = l3.addOrder(order);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 100);
  l3.modifyOrder(order, 50, 101);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 50);
}

TEST(L3levelTest, fillOrderTest) {
  L3PriceLevel l3;
  auto order = std::make_shared<Order>(1, true, 100, 101);
  auto iter = l3.addOrder(order);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 100);
  l3.fillOrder(order, 20);
  EXPECT_EQ(l3.num_orders, 1);
  EXPECT_EQ(l3.quantity, 80);
}

}