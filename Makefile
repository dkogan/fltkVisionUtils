CXXFLAGS += -g -Wall -Wextra -pedantic
LDFLAGS  += -g

LDLIBS += -lfltk -lpthread -ldc1394

test: frameSource.o camera.o main.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f *.o test

deps.d: $(wildcard *.c *.cc *.cpp *.h *.hh)
	@touch $@
	makedepend -f $@ -Y -- $(CFLAGS) $(CXXFLAGS) $(INCLUDE) -- $(wildcard *.c *.cc *.cpp) 2>/dev/null

-include deps.d
