CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
M        ?= 5

TARGET  := btree
SRCS    := main.cpp src/disk_manager.cpp src/btree.cpp src/sorted_array.cpp
OBJS    := $(SRCS:.cpp=.o)

STAMP := .m_stamp_$(M)

TEST_TARGET := tests/stress_btree
TEST_SRCS   := tests/stress_btree.cpp src/disk_manager.cpp src/btree.cpp
TEST_OBJS   := $(TEST_SRCS:.cpp=.o)

BENCH_TARGET := bench/bench
BENCH_SRCS   := bench/bench.cpp src/disk_manager.cpp src/btree.cpp

.PHONY: all clean run tests bench

all: $(STAMP) $(TARGET)

bench:
	$(CXX) $(CXXFLAGS) -DM=$(M) -o $(BENCH_TARGET) $(BENCH_SRCS)

tests: $(STAMP) $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -DM=$(M) -o $@ $^

$(STAMP):
	rm -f .m_stamp_*
	touch $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -DM=$(M) -o $@ $^

%.o: %.cpp $(STAMP)
	$(CXX) $(CXXFLAGS) -DM=$(M) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) *.bin .m_stamp_*
