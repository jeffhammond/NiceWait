CC      =  mpicc

# C99 is _required_ to build
# GNU extensions required for nanosleep
CFLAGS  = -std=gnu11

# obsessive compulsive disorder
CFLAGS += -Wall -Wextra -Wunused -Wformat

# refuse to compile code if there are warnings
CFLAGS += -Werror

# which sleep to use
#CFLAGS += -DHAVE_NANOSLEEP
CFLAGS += -DHAVE_USLEEP
# required for USLEEP
CFLAGS += -DHAVE_UNISTD_H

all: nicewait.o test

nicewait.o : nicewait.c
	$(CC) $(CFLAGS) -c $< -o $@

test : test.c nicewait.o
	$(CC) $(CFLAGS) $< nicewait.o -o $@

check: test
	mpirun -n 4 ./test "bogus" 0 100 "hijinks"

clean:
	-rm -f nicewait.o
	-rm -f test
	-rm -rf *.dSYM

