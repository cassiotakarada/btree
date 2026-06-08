CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
M        ?= 5

TARGET  := btree
SRCS    := main.cpp src/disk_manager.cpp src/btree.cpp
OBJS    := $(SRCS:.cpp=.o)

# Stamp file captures the current M value.
# When M changes, the stamp is outdated and all objects are rebuilt.
STAMP := .m_stamp_$(M)

TEST_TARGET := tests/stress_btree
TEST_SRCS   := tests/stress_btree.cpp src/disk_manager.cpp src/btree.cpp
TEST_OBJS   := $(TEST_SRCS:.cpp=.o)

# Driver de benchmark (avaliação experimental). Compilado direto das fontes
# (sem reaproveitar os .o, que carregam o M do build principal) para que o
# orquestrador possa rebuildar com `make bench M=<ordem>` de forma confiável.
BENCH_TARGET := bench/bench
BENCH_SRCS   := bench/bench.cpp src/disk_manager.cpp src/btree.cpp

.PHONY: all clean run tests bench

all: $(STAMP) $(TARGET)

# Recompila sempre (o M é compile-time e muda entre rodadas de experimento).
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
