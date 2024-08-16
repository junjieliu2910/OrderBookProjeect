# How to compile and run
* Create the build folder and build the tests
```bash
mkdir build
cmake ..
make
```

* Run all the test
```bash
ctest
```

# Implementation
## Assumptions
The implementation is based on following assumptions:
1. All the messages sent by exchange are valid
2. Exchange will handles order by price-time priority (FIFO) manner. And the order arrives first will be published first in L3 order feed also.
3. The messages in each steam are in order
  * For example, with following order book,
  ```
       Bid     Ask
             50@103
             30@101
      20@99
      40@98
  ```
  * When an aggressive limit order B 50@104 is received, following trades are expected to arrive in order 
    * 30@101 is followed by 20@103
  * This will impact the guess of order events when order and trade streams are unsynchronized. This assumption will reduce the number of guessed order events
4. Only limit order will be received, including the liquidity removing orders, which should be aggressive limit orders
5. Order events include ADD/CANCEL/MODIFY/EXECUTION 
   * Order modification cannot change order side (buy or sell)
6. Every order event that will change the L2 book status will trigger a L2 snapshot update

## Liquidity-adding events and Liquidity-removing events
Each order or trade message or snapshot message can be classified as liquidity-adding or liquidity-removing.
* Liquidity-adding events:
  * all order creation
  * some order modification
* Liquidity-removing events:
  * all trade
    * remove liquidity on both sides
  * all order cancellation
    * remove liquidity on one side
  * some order modifications

In this implementation, the order event will be handled on book side (bid or ask side) mainly.
Each side will keep two maps 
* price to pending liquidity-adding quantity
* price to pending liquidity-removing quantity

## Examples
Consider the ask side book with following status 
```bash
  Ask 
40@104.00
80@103.00
60@102.00
50@101.00
60@100.00
```

### When trade stream leads order stream
* Case 1
```bash
  Ask                                                              Ask book after this trade
40@104.00                                                              40@104.00 
80@103.00                                                              80@103.00
60@102.00                                                              60@102.00
50@101.00                                                              50@101.00 
60@100.00  ==> Receive a trade 10@100.00                               50@100.00
```
If a trade 10@100.00 is received, the corresponding liquidity taking order is expected to arrive later. So, bid quantity 10 should be removed from ask level price 100.00.
Meanwhile, on the bid book side, we will add pending liquidity-adding quantity of 10 with price higher or equal to 100.00. 
Since this trade might be triggered by aggressive buy order 10@100.00 or 10@101.00 even 10@200.00. As long as the bid price is higher than or equal to 100.00

* Case 2
```bash
  Ask                                                              Ask book after this trade
40@104.00                                                              40@104.00 
80@103.00                                                              80@103.00
60@102.00  ==> Receive a trade 10@102.00                               50@102.00
50@101.00                                                              
60@100.00                                                              
```
Since we assume that the messages arrive in order in each stream. If a trade 10@102.00 is received, all the ask orders with price 100 and 101 are expected to be cancelled. If some trades with price 100 or 101 exist, they should arrive before the 10@102 as illustrated in assumption 3.
So those orders with price 101 and 100 will be treated as cancelled immediately. And the pending liquidity removing quantity at corresponding price level will be increase to be matched with incoming order cancellation

* Case 3
```bash
  Ask                                                              Ask book after this trade
40@104.00                                                              40@104.00 
80@103.00                                                              80@103.00
60@102.00                                                              60@102.00
50@101.00                                                              50@101.00 
60@100.00  ==> Receive a trade 10@99.00                                50@100.00
```
In this, the trade price is more aggressive than top of the book. Then, some incoming order that providing liquidiy at both sides are expected. The pending liquidity adding quantity will be incrased. 

Assume a ask order 10@99 arrive, it will firstly match with the pending liquidity adding quantity and won't change the current order book

Assume a bid order 10@99 arrive, it will firstly match with the pending liquidity adding quantity and won't change the current order book too. So the order book can be in accurate status.


### When order stream leads trade stream
```bash
  Ask                                                          Ask book after this order
40@104.00                                                              40@104.00
80@103.00                                                              80@103.00
60@102.00                                                              60@102.00
50@101.00  ==> Receive a bid order 101@80                              30@101.00
60@100.00  
```
In this case, the order book might be crossed. The implementation uncrosses the order book immediately and then add the pending liquidity removing quantity for price level 100 and 101

| price | pending liq removing qty |
|-------|--------------------------|
| 100   | 60                       |
| 101   | 20                       |

When trade 60@100 arrives, the corresponding pending quantity will be matched.

| price | pending liq removing qty |
|-------|--------------------------|
| 101   | 20                       |

When trade 20@101 arrive, although the book status is similar to the situation that trade stream leads order steam, the book status won't be updated as case 1 since the order will be matched with the pending liquidity removing quantity first, which keep the book status accurate. 

### When L2 snapshot stream leads order and trade streams
### When order stream leads trade stream
```bash
  Ask                       L2 snapshot                           Ask book after this order
40@104.00                                                             
80@103.00                    80@103.00                                  80@103.00
60@102.00                    50@102.00                                  50@102.00
50@101.00  
60@100.00  
```
In this case, at each limit level, if l2 snapshot quantity > current quantity, then increase the liquidity adding quantity at that leve. Else increase the liquidity removing quantity. The implementation will guess order events to make the order book matches with l2 snapshot received. 

For ask book, the guessing logic is that among all the removed orders. The orders with lower prices will be guessed as executed first until the number of orders guessed as executed compose 30% of overall number of orders removed.


# General message process steps
## When a new order add message arrive
1. Check whether there is pending liquidity adding quantity the can be matched. Remove the matched quantity from current order quantity if there is a match
2. If there is still remaining quantity after step 1. Check whether this order will make the order book crossed. Uncross the book if necessary
3. If there are still remaining quantity after step 2. Add the remaining quantity as normal synchronized order

## When a new order cancel message arrive
1. Match the cancellation quantity with pending liquidity removing quantity first
2. If there is still remaining quantity, cancel and update order book

## When a new order modify message arrive
Order modification is simulated by order cancellation and order creation event in this implementation

## When a new trade arrive
1. Match the trade with pending liquidity removing quantity first
2. If there is still remaining quantity for this trade. Try to handle it by cases as illustrated in the previous section.

## when a new l2 snapshot message arrive
1. Check whether the l2 snapshot leads order / tarde stream
2. If that's the case, match the current order book with l2 snapshot received and guess the liquidity removing events based on the assumption that 30% of the orders will be executed


# Test cases
Tests for BookSide and SmartOrderBook cover the lead-lag cases. Currently, all the tests pass.


# Improvements
* Use custom and optimized memory allocator for std::map to reduce the number of memory allocation
  * Better use a static pool allocator
