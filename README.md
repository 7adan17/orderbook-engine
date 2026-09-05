# Limit Order Book & Matching Engine

A C++17 limit order book with strict **price-time priority** matching.

- Price levels in `std::map` (bids descending, asks ascending), each a FIFO `std::list`
- O(1) cancel through an `id -> iterator` index
- Limit and market (IOC) orders; trades execute at the resting order's price
- `modify` keeps queue position when size goes down and loses it when size goes up
- Integer tick prices (no floating-point comparisons)

```bash
make test    # unit tests
make bench   # 2M random add/cancel operations
```

On an Apple Silicon laptop the benchmark runs at about 10M ops/s.
