CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude

all: build/test build/bench

build/test: src/order_book.cpp tests/test_order_book.cpp include/order_book.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -UNDEBUG src/order_book.cpp tests/test_order_book.cpp -o $@

build/bench: src/order_book.cpp src/bench.cpp include/order_book.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -DNDEBUG src/order_book.cpp src/bench.cpp -o $@

test: build/test
	./build/test

bench: build/bench
	./build/bench

clean:
	rm -rf build

.PHONY: all test bench clean
