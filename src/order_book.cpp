#include "order_book.hpp"

#include <algorithm>
#include <stdexcept>

namespace ob {

template <typename Book>
void OrderBook::match(Book& book, Order& taker, bool is_market, std::vector<Trade>& out) {
    while (taker.qty > 0 && !book.empty()) {
        auto level_it = book.begin();
        const Price level_price = level_it->first;

        if (!is_market) {
            const bool crosses = taker.side == Side::Buy ? level_price <= taker.price
                                                         : level_price >= taker.price;
            if (!crosses) break;
        }

        Queue& queue = level_it->second;
        while (taker.qty > 0 && !queue.empty()) {
            Order& maker = queue.front();
            const Qty fill = std::min(taker.qty, maker.qty);
            out.push_back({maker.id, taker.id, level_price, fill});
            taker.qty -= fill;
            maker.qty -= fill;
            if (maker.qty == 0) {
                index_.erase(maker.id);
                queue.pop_front();
            }
        }
        if (queue.empty()) book.erase(level_it);
    }
}

void OrderBook::rest(const Order& o) {
    if (o.side == Side::Buy) {
        auto& q = bids_[o.price];
        auto it = q.insert(q.end(), o);
        index_[o.id] = {o.side, o.price, it};
    } else {
        auto& q = asks_[o.price];
        auto it = q.insert(q.end(), o);
        index_[o.id] = {o.side, o.price, it};
    }
}

std::vector<Trade> OrderBook::add_limit(OrderId id, Side side, Price price, Qty qty) {
    if (qty == 0) throw std::invalid_argument("quantity must be positive");
    if (index_.count(id)) throw std::invalid_argument("duplicate order id");

    Order taker{id, side, price, qty, next_seq_++};
    std::vector<Trade> trades;
    if (side == Side::Buy) match(asks_, taker, false, trades);
    else match(bids_, taker, false, trades);

    if (taker.qty > 0) rest(taker);
    return trades;
}

std::vector<Trade> OrderBook::add_market(OrderId id, Side side, Qty qty) {
    if (qty == 0) throw std::invalid_argument("quantity must be positive");
    Order taker{id, side, 0, qty, next_seq_++};
    std::vector<Trade> trades;
    if (side == Side::Buy) match(asks_, taker, true, trades);
    else match(bids_, taker, true, trades);
    return trades;
}

bool OrderBook::cancel(OrderId id) {
    auto found = index_.find(id);
    if (found == index_.end()) return false;
    const Locator loc = found->second;
    index_.erase(found);

    auto erase_from = [&](auto& book) {
        auto level_it = book.find(loc.price);
        level_it->second.erase(loc.it);
        if (level_it->second.empty()) book.erase(level_it);
    };
    if (loc.side == Side::Buy) erase_from(bids_);
    else erase_from(asks_);
    return true;
}

bool OrderBook::modify(OrderId id, Qty new_qty) {
    auto found = index_.find(id);
    if (found == index_.end()) return false;
    if (new_qty == 0) return cancel(id);

    Order& o = *found->second.it;
    if (new_qty <= o.qty) {
        o.qty = new_qty;  // keeps queue position
        return true;
    }
    // Size increase loses time priority.
    const Order copy = o;
    cancel(id);
    add_limit(copy.id, copy.side, copy.price, new_qty);
    return true;
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

template <typename Book>
std::vector<Level> OrderBook::depth_of(const Book& book, std::size_t levels) {
    std::vector<Level> out;
    for (auto it = book.begin(); it != book.end() && out.size() < levels; ++it) {
        Qty total = 0;
        for (const auto& o : it->second) total += o.qty;
        out.push_back({it->first, total, it->second.size()});
    }
    return out;
}

std::vector<Level> OrderBook::depth(Side side, std::size_t levels) const {
    return side == Side::Buy ? depth_of(bids_, levels) : depth_of(asks_, levels);
}

}  // namespace ob
