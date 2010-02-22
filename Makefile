CXXFLAGS += -g -Wall -Wextra -pedantic
LDFLAGS  += -g

LDLIBS_CAMERASAMPLE += -lfltk -lpthread -ldc1394
LDLIBS_VIDEOREADERSAMPLE += -lfltk -lavformat -lavcodec -lswscale -lavutil

cameraSample: camera.o cameraSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_CAMERASAMPLE) -o $@

videoreaderSample: ffmpegInterface.o videoreaderSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_VIDEOREADERSAMPLE) -o $@


clean:
	rm -f *.o cameraSample videoreaderSample

deps.d: $(wildcard *.c *.cc *.cpp *.h *.hh)
	@touch $@
	makedepend -f $@ -Y -- $(CFLAGS) $(CXXFLAGS) -- $(wildcard *.c *.cc *.cpp) 2>/dev/null

-include deps.d
