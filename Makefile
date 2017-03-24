.PHONY: all

FLAGS=-g -O2 -Wall -pedantic -Wextra -Werror -Wno-unused-parameter -Wno-long-long

all: test.cpp libpopcnt.h
	$(CXX) $(FLAGS) test.cpp -o test

check: all
	./test

clean:
	rm -f ./test
