#include "order_book.hpp"

#include <cassert>
#include <cstdio>

using namespace ob;

static void test_resting_no_cross() {
    OrderBook book;
    assert(book.add_limit(1, Side::Buy, 100, 10).empty());
    assert(book.add_limit(2, Side::Sell, 101, 10).empty());
    assert(*book.best_bid() == 100);
    assert(*book.best_ask() == 101);
}

static void test_price_priority() {
    OrderBook book;
    book.add_limit(1, Side::Sell, 102, 5);
    book.add_limit(2, Side::Sell, 101, 5);
    auto trades = book.add_limit(3, Side::Buy, 102, 5);
    assert(trades.size() == 1);
    assert(trades[0].maker_id == 2);   // better price fills first
    assert(trades[0].price == 101);    // at the maker's price
}

static void test_time_priority() {
    OrderBook book;
    book.add_limit(1, Side::Buy, 100, 5);
    book.add_limit(2, Side::Buy, 100, 5);
    auto trades = book.add_limit(3, Side::Sell, 100, 7);
    assert(trades.size() == 2);
    assert(trades[0].maker_id == 1 && trades[0].qty == 5);
    assert(trades[1].maker_id == 2 && trades[1].qty == 2);
    assert(book.depth(Side::Buy, 1)[0].total_qty == 3);
}

static void test_partial_fill_rests_remainder() {
    OrderBook book;
    book.add_limit(1, Side::Sell, 100, 3);
    auto trades = book.add_limit(2, Side::Buy, 101, 10);
    assert(trades.size() == 1 && trades[0].qty == 3);
    assert(*book.best_bid() == 101);
    assert(!book.best_ask());
}

static void test_cancel_and_modify() {
    OrderBook book;
    book.add_limit(1, Side::Buy, 100, 5);
    book.add_limit(2, Side::Buy, 100, 5);
    assert(book.modify(1, 2));          // reduce keeps priority
    auto t = book.add_limit(3, Side::Sell, 100, 2);
    assert(t[0].maker_id == 1);
    assert(book.cancel(2));
    assert(!book.cancel(2));
    assert(book.size() == 0);
    assert(!book.best_bid());
}

static void test_modify_increase_loses_priority() {
    OrderBook book;
    book.add_limit(1, Side::Buy, 100, 5);
    book.add_limit(2, Side::Buy, 100, 5);
    book.modify(1, 8);
    auto t = book.add_limit(3, Side::Sell, 100, 1);
    assert(t[0].maker_id == 2);
}

static void test_market_order() {
    OrderBook book;
    book.add_limit(1, Side::Sell, 100, 2);
    book.add_limit(2, Side::Sell, 105, 2);
    auto t = book.add_market(3, Side::Buy, 10);
    assert(t.size() == 2);
    assert(book.size() == 0);  // remainder discarded, not rested
}

int main() {
    test_resting_no_cross();
    test_price_priority();
    test_time_priority();
    test_partial_fill_rests_remainder();
    test_cancel_and_modify();
    test_modify_increase_loses_priority();
    test_market_order();
    std::puts("all order book tests passed");
}
