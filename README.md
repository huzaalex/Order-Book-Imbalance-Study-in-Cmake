# Order book imbalance: does it predict the next price move?

A small C++/Python study on Binance BTCUSDT. Recorded order book snapshots are
replayed through a C++ order book, top-of-book imbalance is computed, and Python
scripts test whether it predicts short-horizon moves of the mid price.

I built it mainly to learn C++ (`std::map`, iterators, `std::optional`, CMake,
testing, benchmarking), so the data-structure work is as much the point as the
statistics.

## Summary

- Imbalance over the top 5 levels predicted the direction of the mid price over
  ~1 second on two separate sessions (15 min and 30 min of data).
- The effect is **real but tiny: about 0.1 bps, mid-to-mid, before fees**.
  It is orders of magnitude below taker fees, so it is **not tradable** with
  market orders.
- A sorted `std::vector` processes a 20-level snapshot about 6-10x faster than
  `std::map` in this workload.

## Question

Does `imbalance = (bid_volume - ask_volume) / (bid_volume + ask_volume)` over the
top N levels predict the direction of the mid price a short time later?

## Data

`scripts/record.py` records the `btcusdt@depth20@100ms` stream (a full top-20
snapshot every 100 ms) into a CSV with one row per level:

```
ts_ms,side,price_ticks,qty
```

- Prices are stored as integer ticks (tick = 0.01), quantities scaled by 1e8,
  converted with `Decimal` so no floating point enters the data.
- `ts_ms` is the **local receive time**, not an exchange timestamp.
- All rows with the same `ts_ms` form one snapshot.
- Session 1: 15 minutes, 8,811 snapshots. Session 2: 30 minutes, 17,952
  snapshots. Recorded on [fill: dates / time of day].

## Order book (C++)

`ob::OrderBook` keeps two `std::map`s (bids descending, asks ascending) with
integer prices and quantities. Missing values (empty book or side) are
`std::optional`. `replay` rebuilds the book for every snapshot (`clear()`, then
`update()` per level) and writes mid, spread and imbalance at 1, 5 and 20 levels
to a CSV.

`ob::VecBook` is an alternative with the same interface, backed by sorted
`std::vector`s and `std::lower_bound`. It exists for the benchmark below.

Testing:
- Unit tests (`assert` + CTest): empty book, one-sided book, overwrite, removal,
  removal of a missing level, ordering, `clear`, imbalance edge cases.
- A randomized test compares `OrderBook` and `VecBook` after each of 100,000
  random updates.
- `scripts/check.py` recomputes imbalance in Python from the raw CSV and compares
  it with the C++ output (identical to 6 decimals on the snapshots checked).

## Evaluation

**Session 1 (exploration).** `analyze.py` and `diag.py` looked at correlation,
direction hit rate and mean signed move for imbalance at 1/5/20 levels and
horizons of 1-50 snapshots (15 signal/horizon combinations). Findings:
- Correlation with the next mid move was positive (about +0.19 for 5 levels at a
  10-snapshot horizon).
- The mid fell by about 5,300 ticks over the session while imbalance was
  positive 73% of the time, and "always long" lost, so a simple drift does not
  explain the effect.
- Delaying entry by up to 500 ms (5 snapshots) reduced the edge only from
  about +33 to +29 ticks.
- The edge had the same sign in each third of the session.

**Frozen parameters.** I chose one configuration and fixed it before evaluating
session 2:

| Parameter | Value |
|---|---|
| signal | `imb5` (top 5 levels), centred at the session-1 median 0.673 |
| horizon K | 10 snapshots (~1 s) |
| execution delay L | 3 snapshots (~300 ms) |

I also wrote the acceptance criterion before running it: positive edge in all
three 10-minute chunks, similar order of magnitude to session 1.

**Session 2 (hold-out), `scripts/eval_holdout.py`, run once:**

| | |
|---|---|
| mean edge | 60.5 ticks (0.099 bps) |
| edge per 10-min chunk | 67.9 / 62.1 / 51.4 ticks |
| fraction of time long | 0.36 |
| non-overlapping windows | n = [fill], t = [fill] |

The criterion was met. The edge is larger in ticks than in session 1, but ticks
depend on volatility, so I do not read this as a stronger signal.

**Costs.** The edge is measured mid-to-mid, before fees. The spread is about
1 tick (~0.002 bps) and is negligible, but taker fees are [fill: your taker fee
in bps] per side. Even at 1 bp per side the edge would be tens of times smaller
than the cost of a round trip.

## Benchmark

Time to process one snapshot (`clear` + 40 `update` + `imbalance(5)`) on 17,952
snapshots of session 2 (718,080 rows), minimum of 7 runs. Release build, GCC
16.2.0 (MinGW-w64), C++20, [CPU:11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz], Windows.

| Level order in the input | `std::map` | sorted `std::vector` | speedup |
|---|---|---|---|
| as recorded (best level first) | 3183 ns | 324 ns | 9.8x |
| reversed (worst case for the vector) | 3224 ns | 502 ns | 6.4x |

Medians: 4450 / 505 ns (as recorded) and 3450 / 598 ns (reversed).

- Both books give identical results: the checksum (sum of imbalances) is the
  same in every run.
- Both are far below the 100 ms between snapshots, so performance is not a
  practical bottleneck. This is a measurement exercise.
- Likely cause (a hypothesis, not tested): `std::map` allocates one node per level
  and scatters nodes in memory; the vector is contiguous and keeps its capacity
  after `clear()`.
- Reversing the level order makes every vector insert shift elements, which costs
  about 55%, yet the vector is still ~6x faster at 20 levels.

## Limitations

- Two sessions, one asset, probably similar market conditions. No evidence for
  other regimes or symbols.
- Observations overlap in time and are not independent; effective sample size is
  much smaller than the number of snapshots.
- 15 combinations were tried on session 1, so its numbers are optimistic. Only
  session 2 is a clean test, and it was used once.
- Local receive timestamps; no exchange timestamps or network latency model.
- Edge is measured on the mid price. No fees, queue position, fill probability
  or market impact are modelled.
- Edge in ticks depends on volatility; use scale-free metrics when comparing
  sessions.
- Benchmark conclusions hold for 20-level snapshots and this input order. A deeper
  book fed by a diff stream could change the result.

## How to run

Requires CMake >= 3.20, a C++20 compiler, and Python 3 with
`websocket-client`, `pandas`, `numpy`, `matplotlib`.

```
python -m pip install websocket-client pandas numpy matplotlib

# record data (edit DURATION_S and OUT in the script first)
python scripts/record.py

# build (Release) and replay
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
build/replay.exe data/session1.csv data/features.csv

# unit tests need asserts, so use a Debug build
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure

# analysis
python scripts/check.py
python scripts/analyze.py
python scripts/diag.py
python scripts/eval_holdout.py

# benchmark
build/bench.exe data/session2.csv [reverse]
```

Raw data is not tracked by git; record your own with `record.py`.

## Layout

```
include/ob/   order_book.hpp, vec_book.hpp
src/          order_book.cpp, vec_book.cpp, main.cpp (replay), bench.cpp
tests/        test_order_book.cpp
scripts/      record.py, check.py, analyze.py, diag.py, eval_holdout.py
```

## Next steps

- Use the diff-based `@depth` stream with snapshot synchronisation, so updates
  arrive incrementally and out of order, and re-run the `map` vs `vector`
  comparison there.
- More sessions, other symbols, and exchange timestamps.
- A limit-order model with fill probability, instead of mid-to-mid edge.