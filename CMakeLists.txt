cmake_minimum_required(VERSION 3.20)
project(orderbook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ob src/order_book.cpp src/vec_book.cpp)
target_include_directories(ob PUBLIC include)

add_executable(replay src/main.cpp)
target_link_libraries(replay PRIVATE ob)

add_executable(bench src/bench.cpp)
target_link_libraries(bench PRIVATE ob)

include(CTest)

add_executable(tests tests/test_order_book.cpp)
target_link_libraries(tests PRIVATE ob)
add_test(NAME order_book_tests COMMAND tests)
