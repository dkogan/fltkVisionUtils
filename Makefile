CXXFLAGS += -g -Wall -Wextra -pedantic
LDFLAGS  += -g

LDLIBS_CAMERASAMPLE += -lfltk -lpthread -ldc1394
LDLIBS_VIDEO_CV_SAMPLE += -lfltk -lavformat -lavcodec -lswscale -lavutil -lcv

all: cameraSample videoCvSample

cameraSample: camera.o cameraSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_CAMERASAMPLE) -o $@

videoCvSample: ffmpegInterface.o videoCvSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_VIDEO_CV_SAMPLE) -o $@


clean:
	rm -f *.o cameraSample videoCvSample

deps.d: $(wildcard *.c *.cc *.cpp *.h *.hh)
	@touch $@
	makedepend -f $@ -Y -- $(CFLAGS) $(CXXFLAGS) -- $(wildcard *.c *.cc *.cpp) 2>/dev/null

-include deps.d
