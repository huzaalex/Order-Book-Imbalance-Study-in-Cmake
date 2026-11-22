#include <ob/order_book.hpp>

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

struct Row {
    long long ts;
    ob::Side side;
    ob::Price price;
    ob::Qty qty;
};

bool parse_line(const std::string& line, Row& row) {
    std::istringstream ss(line);
    std::string ts, side, price, qty;
    if (!std::getline(ss, ts, ',') || !std::getline(ss, side, ',') ||
        !std::getline(ss, price, ',') || !std::getline(ss, qty)) {
        return false;
    }
    if (side != "B" && side != "A") return false;
    try {
        row.ts = std::stoll(ts);
        row.side = (side == "B") ? ob::Side::Bid : ob::Side::Ask;
        row.price = std::stoll(price);
        row.qty = std::stoll(qty);
    } catch (...) {
        return false;  // linie ruptă (de ex. ultima, dacă ai oprit cu Ctrl+C)
    }
    return true;
}

// Scrie o linie de features pentru starea curentă a book-ului.
// Returnează false dacă book-ul nu poate da toate valorile (ex. o parte goală).
bool write_features(std::ofstream& out, long long ts, const ob::OrderBook& book) {
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    auto i1 = book.imbalance(1);
    auto i5 = book.imbalance(5);
    auto i20 = book.imbalance(20);
    if (!bid || !ask || !i1 || !i5 || !i20) return false;

    double mid = (static_cast<double>(*bid) + static_cast<double>(*ask)) / 2.0;
    out << ts << ',' << mid << ',' << (*ask - *bid) << ','
        << *i1 << ',' << *i5 << ',' << *i20 << '\n';
    return true;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: replay <input.csv> <output.csv>\n"; //citeste programul linie cu linie, si scrie features in output.csv
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "cannot open " << argv[1] << '\n';
        return 1;
    }
    std::ofstream out(argv[2]);
    out << std::setprecision(10);
    out << "ts_ms,mid_ticks,spread_ticks,imb1,imb5,imb20\n";

    ob::OrderBook book;
    std::string line;
    std::getline(in, line);  // sare antetul

    long long cur_ts = 0;
    bool have_snapshot = false;
    ob::Side prev_side = ob::Side::Bid;
    std::size_t written = 0, skipped = 0, bad_lines = 0;

    while (std::getline(in, line)) {
        Row r;
        if (!parse_line(line, r)) {
            ++bad_lines;
            continue;
        }
        // Snapshot nou: se schimbă ts-ul SAU trecem de la Ask înapoi la Bid
        // (fiecare snapshot scrie întâi toate Bid, apoi toate Ask).
        bool new_snapshot = !have_snapshot || r.ts != cur_ts ||
                            (r.side == ob::Side::Bid && prev_side == ob::Side::Ask);
        if (new_snapshot) {
            if (have_snapshot) {
                if (write_features(out, cur_ts, book)) ++written; else ++skipped;
            }
            book.clear();
            cur_ts = r.ts;
            have_snapshot = true;
        }
        book.update(r.side, r.price, r.qty);
        prev_side = r.side;
    }
    if (have_snapshot) {
        if (write_features(out, cur_ts, book)) ++written; else ++skipped;
    }

    std::cout << "snapshots written: " << written << ", skipped: " << skipped
              << ", bad lines: " << bad_lines << '\n';
    return 0;
}