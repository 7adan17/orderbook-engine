// Throughput benchmark: random limit orders and cancels around a mid price.
#include "order_book.hpp"

#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

int main() {
    constexpr int kOps = 2'000'000;
    ob::OrderBook book;
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int> side_d(0, 1), offset_d(-20, 20), qty_d(1, 100), action_d(0, 9);
    std::vector<ob::OrderId> live;
    live.reserve(kOps);

    std::size_t trades = 0;
    auto start = std::chrono::steady_clock::now();
    for (ob::OrderId id = 1; id <= kOps; ++id) {
        if (action_d(rng) < 3 && !live.empty()) {
            std::size_t i = rng() % live.size();
            book.cancel(live[i]);
            live[i] = live.back();
            live.pop_back();
        } else {
            auto side = side_d(rng) ? ob::Side::Buy : ob::Side::Sell;
            trades += book.add_limit(id, side, 10'000 + offset_d(rng), qty_d(rng)).size();
            live.push_back(id);
        }
    }
    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("%d ops in %.3fs  (%.2f M ops/s, %zu trades, %zu resting)\n",
                kOps, secs, kOps / secs / 1e6, trades, book.size());
}
