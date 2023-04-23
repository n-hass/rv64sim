CC=gcc
CXX=g++
RM=rm -rf
CPPFLAGS=-g -std=c++11 -Wall -pedantic
LDFLAGS=-g
BOOST=-I/opt/homebrew/include -L/opt/homebrew/lib -lboost_system -lboost_filesystem
LDLIBS=

SRCS=rv64sim.cpp commands.cpp memory.cpp processor.cpp  
OBJS=$(subst .cpp,.o,$(SRCS))

all: rv64sim

rv64simd: $(OBJS)
	$(CXX) $(LDFLAGS) -o rv64simd $(OBJS) $(BOOST) -D ENABLE_BOOST

rv64sim: $(OBJS)
	$(CXX) $(LDFLAGS) -o rv64sim $(OBJS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS) *.dSYM sim.log

dist-clean: clean
	$(RM) *~ .dependtool

include .depend
