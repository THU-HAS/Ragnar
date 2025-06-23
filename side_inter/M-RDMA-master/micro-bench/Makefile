
CC = g++
#CXXFLAGS = -std=c++11

LIBS = -pthread -lnuma
#BENCHMARKS = two-thread-read two-thread-write rw-cycle-bench barrier-bench test-prefetch
BENCHMARKS = two-thread-read two-thread-write test-mem-rand-seq vector-io

all: $(BENCHMARKS)

% : %.cc
	$(CC) -std=c++11 -O3 -o $@ $< $(LIBS)

.PHONY: clean

clean:
	rm -f *.o $(BENCHMARKS)
