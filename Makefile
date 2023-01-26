
# Makefile for Ciel
#
# NOTE: The -O2 optimized version of cl (called clo) dumps core on OS X.
# Not sure why.
#

OS = $(shell sh -c 'uname')
LBITS = $(shell getconf LONG_BIT)

ifeq ($(OS), Linux)
  SHARED_LIBRARY_FLAGS = -shared -fPIC
else ifeq ($(OS), Darwin)
  SHARED_LIBRARY_FLAGS = -bundle -undefined dynamic_lookup  # go figure
endif

CC = g++

ifeq ($(LBITS),64)
   CXXFLAGS = -m64 -Wall -rdynamic
else
   CXXFLAGS = -m32 -Wall -rdynamic
endif

LIBS= -lcln -lgc -ldl -lreadline -ltermcap # or -lncurses, dealer choice
OBJS = rlstream.o stopwatch.o reader.o primops.o

cl: cl.cc cl.h ${OBJS}
	${CC} ${CXXFLAGS} cl.cc ${OBJS} -g -o cl ${LIBS}

clo: cl.cc cl.h ${OBJS}
	${CC} ${CXXFLAGS} -O2 cl.cc ${OBJS} -o clo ${LIBS}

primops.o: primops.cc cl.h
	${CC} ${CXXFLAGS} -O2 -c primops.cc

reader.o: reader.cc reader.h cl.h rlstream.h
	${CC} ${CXXFLAGS} -O2 -c reader.cc

rlstream.o: rlstream.cc rlstream.h
	${CC} ${CXXFLAGS} -O2 -c rlstream.cc -DUSE_GNU_READLINE

stopwatch.o: stopwatch.cc stopwatch.h
	${CC} ${CXXFLAGS} -O2 -c stopwatch.cc

fftest: fftest.cc cl.h
	${CC} ${CXXFLAGS} $(SHARED_LIBRARY_FLAGS) -o fftest fftest.cc

test:
	time ./cl<testdata
	time ./clo<testdata

clean:
	rm -f *.o *~ cl clo fftest
	rm -rf cl.dSYM

all: cl clo fftest test
