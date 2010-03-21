CXXFLAGS += -g -Wall -Wextra -pedantic -MMD
LDFLAGS  += -g -lX11 -lXft -lXinerama

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS_CAMERASAMPLE += -lfltk -lcv -lpthread -ldc1394 $(FFMPEG_LIBS)
LDLIBS_VIDEO_CV_SAMPLE += -lfltk -lcv $(FFMPEG_LIBS)

all: fltkVisionUtils.a cameraSample videoCvSample

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)

fltkVisionUtils.a: $(patsubst %.cc, %.o, $(LIBRARY_SOURCES));
	ar rcvu $@ $^

cameraSample: ffmpegInterface.o cameraSource.o cameraSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_CAMERASAMPLE) -o $@

videoCvSample: ffmpegInterface.o videoCvSample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS_VIDEO_CV_SAMPLE) -o $@


clean:
	rm -f *.o *.a cameraSample videoCvSample

-include *.d