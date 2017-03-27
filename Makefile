.PHONY: all check clean

CXXFLAGS=-g -O2 -Wall -pedantic -Wextra -Werror -Wno-unused-parameter -Wno-long-long

all: test benchmark

%: %.cpp libpopcnt.h
	$(CXX) -o $@ $< $(CXXFLAGS)

check: test
	./test

clean:
	rm -f ./test ./benchmark
