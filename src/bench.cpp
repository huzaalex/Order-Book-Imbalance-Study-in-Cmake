#include <ob/order_book.hpp>
#include <ob/vec_book.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Row { long long ts; ob::Side side; ob::Price price; ob::Qty qty; };

std::vector<Row> load(const char* path) {
    std::ifstream in(path);
    std::vector<Row> rows;
    std::string line;
    std::getline(in, line);  // antet
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string ts, side, price, qty;
        if (!std::getline(ss, ts, ',') || !std::getline(ss, side, ',') ||
            !std::getline(ss, price, ',') || !std::getline(ss, qty)) continue;
        try {
            rows.push_back({std::stoll(ts), side == "B" ? ob::Side::Bid : ob::Side::Ask,
                            std::stoll(price), std::stoll(qty)});
        } catch (...) {}
    }
    return rows;
}

// Returnează ns / snapshot. Citirea fișierului NU e măsurată (rows e deja în memorie).
template <class Book>
double run(const std::vector<Row>& rows, double& sink) {
    Book book;
    std::size_t snapshots = 0;
    long long cur_ts = 0;
    bool have = false;
    ob::Side prev = ob::Side::Bid;

    auto t0 = std::chrono::steady_clock::now();
    for (const Row& r : rows) {
        bool fresh = !have || r.ts != cur_ts ||
                     (r.side == ob::Side::Bid && prev == ob::Side::Ask);
        if (fresh) {
            if (have) sink += book.imbalance(5).value_or(0.0);
            book.clear();
            cur_ts = r.ts;
            have = true;
            ++snapshots;
        }
        book.update(r.side, r.price, r.qty);
        prev = r.side;
    }
    sink += book.imbalance(5).value_or(0.0);
    auto t1 = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::nano>(t1 - t0).count() / double(snapshots);
}

template <class Book>
void bench(const char* name, const std::vector<Row>& rows, double& sink) {
    std::vector<double> t;
    for (int i = 0; i < 7; ++i) t.push_back(run<Book>(rows, sink));
    std::sort(t.begin(), t.end());
    std::cout << name << ": min " << t.front() << " ns, median " << t[t.size() / 2]
              << " ns per snapshot\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: bench <input.csv> [reverse]\n"; return 1; }
    auto rows = load(argv[1]);
    bool reverse_levels = argc >= 3 && std::string(argv[2]) == "reverse";
    if (reverse_levels) {
        for (std::size_t i = 0; i < rows.size();) {
            std::size_t j = i;
            while (j < rows.size() && rows[j].ts == rows[i].ts && rows[j].side == rows[i].side) ++j;
            std::reverse(rows.begin() + i, rows.begin() + j);
            i = j;
        }
    }
    std::cout << rows.size() << " rows loaded" << (reverse_levels ? " (levels reversed)" : "") << "\n";

    double sink = 0;   // împiedică compilatorul să elimine update-urile
    bench<ob::OrderBook>("map", rows, sink);
    std::cout << "  checksum " << sink << "\n";
    sink = 0;
    bench<ob::VecBook>("vector", rows, sink);
    std::cout << "  checksum " << sink << "\n";
}