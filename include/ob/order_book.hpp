#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>

namespace ob {

using Price = std::int64_t; //sa nu dea eroare la virgula, folosim int64_t pentru preț (ex. 123.45 -> 12345)
using Qty   = std::int64_t; //nu double, pentru că nu vrem să pierdem precizie la cantitate (ex. 0.00012345 -> 12345)

enum class Side { Bid, Ask };

class OrderBook {
public:
    void update(Side side, Price price, Qty qty);
    void clear() { bids_.clear(); asks_.clear(); }

    std::optional<Price> best_bid() const; //pt cazurile in care nu exista bid sau ask, returnam std::nullopt
    std::optional<Price> best_ask() const;
    std::optional<Price> spread() const;
    std::optional<double> imbalance(std::size_t n_levels) const;

private:
    std::map<Price, Qty, std::greater<>> bids_;
    std::map<Price, Qty> asks_;
};

}