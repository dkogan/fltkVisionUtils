# debian places the headers and libraries in a different (much more sensible) location than the
# upstream opencv builds. This variable selects this
OPENCV_DEBIAN_PACKAGES = 1

# needed for newer ffmpeg
CXXFLAGS=-D__STDC_CONSTANT_MACROS

CXXFLAGS += -g -O3 -Wall -Wextra -pedantic -MMD
LDFLAGS  += -g
LDLIBS   += -lX11 -lXft -lXinerama

ifeq ($(OPENCV_DEBIAN_PACKAGES), 0)
  OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui
else
  CXXFLAGS += -DOPENCV_DEBIAN_PACKAGES
  CFLAGS   += -DOPENCV_DEBIAN_PACKAGES
  OPENCV_LIBS = -lcv -lcxcore -lhighgui
endif

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += -lfltk $(OPENCV_LIBS) -lpthread -ldc1394 $(FFMPEG_LIBS)

all: fltkVisionUtils.a sample

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)
LIBRARY_OBJECTS = $(patsubst %.cc, %.o, $(LIBRARY_SOURCES))
fltkVisionUtils.a: $(LIBRARY_OBJECTS)
	ar rcvu $@ $^

sample: sample.o fltkVisionUtils.a
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@


clean:
	rm -f *.o *.a *.d sample

-include *.d
