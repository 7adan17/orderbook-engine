#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ob {

using OrderId = std::uint64_t;
using Price = std::int64_t;   // integer ticks, avoids floating-point price comparisons
using Qty = std::uint64_t;

enum class Side { Buy, Sell };

struct Order {
    OrderId id;
    Side side;
    Price price;
    Qty qty;          // remaining quantity
    std::uint64_t seq;  // arrival sequence number, used for time priority
};

struct Trade {
    OrderId maker_id;
    OrderId taker_id;
    Price price;      // trades execute at the resting (maker) order's price
    Qty qty;
};

struct Level {
    Price price;
    Qty total_qty;
    std::size_t order_count;
};

// Limit order book with strict price-time priority.
//
// Each price level is a FIFO queue (std::list) so earlier orders at the same
// price fill first. An id -> iterator index gives O(1) cancels.
class OrderBook {
public:
    // Submits a limit order. Crosses against the opposite side first, then
    // rests any remainder. Returns the trades generated.
    std::vector<Trade> add_limit(OrderId id, Side side, Price price, Qty qty);

    // Market order: crosses until filled or the opposite side is empty.
    // Any unfilled remainder is discarded (IOC semantics).
    std::vector<Trade> add_market(OrderId id, Side side, Qty qty);

    bool cancel(OrderId id);

    // Reduces quantity in place, keeping time priority. Increasing quantity
    // would be unfair to orders behind it, so that is treated as cancel + new.
    bool modify(OrderId id, Qty new_qty);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::vector<Level> depth(Side side, std::size_t levels) const;
    std::size_t size() const { return index_.size(); }

private:
    using Queue = std::list<Order>;
    using Bids = std::map<Price, Queue, std::greater<Price>>;  // highest first
    using Asks = std::map<Price, Queue, std::less<Price>>;     // lowest first

    struct Locator {
        Side side;
        Price price;
        Queue::iterator it;
    };

    template <typename Book>
    void match(Book& book, Order& taker, bool is_market, std::vector<Trade>& out);

    template <typename Book>
    static std::vector<Level> depth_of(const Book& book, std::size_t levels);

    void rest(const Order& o);

    Bids bids_;
    Asks asks_;
    std::unordered_map<OrderId, Locator> index_;
    std::uint64_t next_seq_ = 0;
};

}  // namespace ob
