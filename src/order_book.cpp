#include <ob/order_book.hpp>

namespace ob {

void OrderBook::update(Side side, Price price, Qty qty) {
    if (side == Side::Bid) {
        if (qty == 0) bids_.erase(price); else bids_[price] = qty;
    } else {
        if (qty == 0) asks_.erase(price); else asks_[price] = qty;
    }
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    if (bids_.empty() || asks_.empty()) return std::nullopt;
    return asks_.begin()->first - bids_.begin()->first;
}

std::optional<double> OrderBook::imbalance(std::size_t n_levels) const {
    Qty bid_vol = 0;
    std::size_t i = 0;
    for (auto it = bids_.begin(); it != bids_.end() && i < n_levels; ++it, ++i) {
        bid_vol += it->second;
    }

    Qty ask_vol = 0;
    i = 0;
    for (auto it = asks_.begin(); it != asks_.end() && i < n_levels; ++it, ++i) {
        ask_vol += it->second;
    }

    if (bid_vol + ask_vol == 0) return std::nullopt;
    return static_cast<double>(bid_vol - ask_vol) / static_cast<double>(bid_vol + ask_vol);
}

}