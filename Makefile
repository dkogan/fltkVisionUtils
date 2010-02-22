CXXFLAGS += -g -Wall -Wextra -pedantic
LDFLAGS  += -g

LDLIBS_CAMERASAMPLE += -lfltk -lpthread -ldc1394

cameraSample: camera.o cameraSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f *.o cameraSample

deps.d: $(wildcard *.c *.cc *.cpp *.h *.hh)
	@touch $@
	makedepend -f $@ -Y -- $(CFLAGS) $(CXXFLAGS) -- $(wildcard *.c *.cc *.cpp) 2>/dev/null

-include deps.d
