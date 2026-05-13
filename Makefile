CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
M        ?= 5

TARGET  := btree
SRCS    := main.cpp src/disk_manager.cpp src/btree.cpp
OBJS    := $(SRCS:.cpp=.o)

# Stamp file captures the current M value.
# When M changes, the stamp is outdated and all objects are rebuilt.
STAMP := .m_stamp_$(M)

.PHONY: all clean run

all: $(STAMP) $(TARGET)

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
	rm -f $(OBJS) $(TARGET) *.bin .m_stamp_*
