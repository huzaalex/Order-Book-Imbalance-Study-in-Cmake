#include <ob/order_book.hpp>
#include <iostream>
#include <cassert>
#include <random>
#include <ob/vec_book.hpp>

using namespace ob;
//am adaugat mai multe teste pentru a verifica functionalitatea OrderBook
void test_empty_book() {
    OrderBook book;
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(!book.spread().has_value());
}
void test_vec_matches_map() {
    OrderBook a;
    VecBook b;
    std::mt19937_64 rng(42);
    for (int i = 0; i < 100000; ++i) {
        Side side = (rng() % 2) ? Side::Bid : Side::Ask;
        Price price = 1000 + static_cast<Price>(rng() % 30);
        Qty qty = static_cast<Qty>(rng() % 4);   // 0 șterge nivelul
        a.update(side, price, qty);
        b.update(side, price, qty);
        assert(a.best_bid() == b.best_bid());
        assert(a.best_ask() == b.best_ask());
        assert(a.imbalance(5) == b.imbalance(5));
    }
}
void test_best_prices_and_spread() {
    OrderBook book;
    book.update(Side::Bid, 100, 5);
    book.update(Side::Bid, 101, 3);
    book.update(Side::Ask, 103, 4);
    assert(*book.best_bid() == 101);
    assert(*book.best_ask() == 103);
    assert(*book.spread() == 2);
}

void test_remove_level() {
    OrderBook book;
    book.update(Side::Bid, 100, 5);
    book.update(Side::Bid, 101, 3);
    book.update(Side::Bid, 101, 0);
    assert(*book.best_bid() == 100);
}

void test_overwrite_level() {
    OrderBook book;
    book.update(Side::Bid, 100, 5);
    book.update(Side::Bid, 100, 9);  // același preț, altă cantitate
    book.update(Side::Bid, 100, 0);  // dacă era dublat, ar mai exista un nivel
    assert(!book.best_bid().has_value());
}

void test_remove_nonexistent_level() {
    OrderBook book;
    book.update(Side::Ask, 105, 2);
    book.update(Side::Ask, 999, 0);  // nu există, nu trebuie să crape nimic
    assert(*book.best_ask() == 105);
}

void test_one_sided_book() {
    OrderBook book;
    book.update(Side::Bid, 100, 5);
    assert(book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(!book.spread().has_value());
}

void test_ask_ordering() {
    OrderBook book;
    book.update(Side::Ask, 110, 1);
    book.update(Side::Ask, 105, 1);
    book.update(Side::Ask, 108, 1);
    assert(*book.best_ask() == 105);  // cel mai mic preț
    book.update(Side::Ask, 105, 0);
    assert(*book.best_ask() == 108);
}

void test_clear() {
    OrderBook book;
    book.update(Side::Bid, 100, 5);
    book.update(Side::Ask, 103, 4);
    book.clear();
    assert(!book.best_bid().has_value());
    assert(!book.best_ask().has_value());
    assert(!book.imbalance(5).has_value());
}

int main() {
    test_empty_book();
    test_best_prices_and_spread();
    test_remove_level();
    test_overwrite_level();
    test_remove_nonexistent_level();
    test_one_sided_book();
    test_ask_ordering();
    std::cout << "All tests passed\n";
    return 0;
}