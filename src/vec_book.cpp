#include <ob/vec_book.hpp>
#include <algorithm>

namespace ob {

std::optional<Price> VecBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.front().first;   // vectorul e sortat descrescător
}

std::optional<Price> VecBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.front().first;   // vectorul e sortat crescător
}

std::optional<double> VecBook::imbalance(std::size_t n_levels) const {
    Qty bid_vol = 0;
    for (std::size_t i = 0; i < std::min(n_levels, bids_.size()); ++i) {
        bid_vol += bids_[i].second;
    }
    Qty ask_vol = 0;
    for (std::size_t i = 0; i < std::min(n_levels, asks_.size()); ++i) {
        ask_vol += asks_[i].second;
    }
    if (bid_vol + ask_vol == 0) return std::nullopt;
    return static_cast<double>(bid_vol - ask_vol) / static_cast<double>(bid_vol + ask_vol);
}

void VecBook::update(Side side, Price price, Qty qty) {
    auto& vec = (side == Side::Bid) ? bids_ : asks_;
    auto it = std::lower_bound(vec.begin(), vec.end(), price,
        [side](const Level& level, Price p) {
            return (side == Side::Bid) ? (level.first > p) : (level.first < p);
        });
    if (it != vec.end() && it->first == price) {
        if (qty == 0) {
            vec.erase(it);
        } else {
            it->second = qty;
        }
    } else {
        if (qty > 0) {
            vec.insert(it, {price, qty});
        }
    }
}

}