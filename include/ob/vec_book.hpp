#pragma once
#include <ob/order_book.hpp>   // de aici vin Price, Qty, Side
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace ob {

class VecBook {
public:
    void update(Side side, Price price, Qty qty);
    void clear() { bids_.clear(); asks_.clear(); }

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::optional<double> imbalance(std::size_t n_levels) const;

private:
    using Level = std::pair<Price, Qty>;
    std::vector<Level> bids_;   // sortat descrescător după preț
    std::vector<Level> asks_;   // sortat crescător
};

}