# selects opencv 2.1 or 2.0
OPENCV_VERSION = 200

CXXFLAGS += -g -O3 -Wall -Wextra -pedantic -MMD
CXXFLAGS += -DOPENCV_VERSION=$(OPENCV_VERSION)
LDFLAGS  += -g -lX11 -lXft -lXinerama

ifeq ($(OPENCV_VERSION), 210)
  OPENCV_LIBS = -lopencv_core -lopencv_imgproc
else
  OPENCV_LIBS = -lcv
endif

FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += -lfltk $(OPENCV_LIBS) -lpthread -ldc1394 $(FFMPEG_LIBS)

all: fltkVisionUtils.a sample

# all non-sample .cc files
LIBRARY_SOURCES = $(shell ls *.cc | grep -v -i sample)
LIBRARY_OBJECTS = $(patsubst %.cc, %.o, $(LIBRARY_SOURCES))
fltkVisionUtils.a: $(LIBRARY_OBJECTS)
	ar rcvu $@ $^

sample: $(LIBRARY_OBJECTS) sample.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@


clean:
	rm -f *.o *.a *.d sample

-include *.d
