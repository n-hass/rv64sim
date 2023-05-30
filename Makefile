CC=gcc
CXX=g++
RM=rm -rf
CPPFLAGS=-g -std=c++11 -Wall -pedantic -O3
LDFLAGS=-g -O3
LDLIBS=

ifdef LOGGING
CPPFLAGS+= -DLOGGING_ENABLED
LDFLAGS+= -DLOGGING_ENABLED
endif

SRCS=rv64sim.cpp commands.cpp memory.cpp processor.cpp  
OBJS=$(subst .cpp,.o,$(SRCS))

all: rv64sim

rv64sim: $(OBJS)
	$(CXX) $(LDFLAGS) -o rv64sim $(OBJS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS) *.dSYM sim.log
	$(RM) tests/*_tests/*.log

dist-clean: clean
	$(RM) *~ .dependtool

include .depend
