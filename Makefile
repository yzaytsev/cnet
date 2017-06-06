OBJECTS = 	detection/paging.o \
		detection/cache.o \
		detection/cpu.o \
		util/error.o \
		util/timing.o \
		util/colorprint.o \
		util/getopt_helper.o \
		util/watchdog.o \
		cache/evict.o \
		cache/set.o \
		cache/slice.o \
		jag/send.o \
		jag/common.o \
		jag/receive.o \
		net/transfer.o \
		net/codemap.o \
		net/encoder.o

TEST_OBJECTS = \
		net/encoder.o \
		net/codemap.o

#FLAGS = -Wall -g -O3 -march=native -std=gnu11 -pthread
#FLAGS = -Wall -g -O0 -msoft-float -march=ivybridge -std=gnu11 -pthread
FLAGS = -Wall -g -O3 -march=core2 -std=gnu11 -pthread -I.

all: cnet

build_test: test
	
test: test.c $(TEST_OBJECTS)
	gcc $(OBJECTS) $(FLAGS) -o $@ $@.c -lm

cnet: cnet.c $(OBJECTS) cnet.h
	gcc $(OBJECTS) $(FLAGS) -o $@ $@.c -lm

%.o: %.c
	gcc $(FLAGS) -c $^ -o $@

net/codemap.c : net/codemap.h net/gen_table.rb
	./net/gen_table.rb > $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) cnet
