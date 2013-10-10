# needed for newer ffmpeg
CXXFLAGS=-D__STDC_CONSTANT_MACROS

CXXFLAGS += -g -O3 -Wall -Wextra -pedantic -MMD
LDFLAGS  += -g
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui
FFMPEG_LIBS = -lavformat -lavcodec -lswscale -lavutil
LDLIBS += -lfltk $(OPENCV_LIBS) -lpthread -ldc1394 $(FFMPEG_LIBS)


API_VERSION := 1
VERSION     := $(shell perl -ne 's/.*\((.*?)\).*/$$1/; print; exit' debian/changelog)
SO_VERSION  := $(API_VERSION).$(VERSION)

TARGET_SO := libvisionio.so.$(SO_VERSION)
TARGET_A  := libvisionio.a

all: $(TARGET_A) $(TARGET_SO) sample


LIB_OBJECTS = $(patsubst %.cc,%.o,$(filter-out sample,$(wildcard *.cc)))

$(TARGET_SO): $(LIB_OBJECTS:%.o=%-fpic.o)
	$(CXX) -shared  $^ $(LDLIBS) -Wl,-soname -Wl,libvisionio.so.$(API_VERSION) -Wl,--copy-dt-needed-entries -o $@

%-fpic.o: CXXFLAGS += -fPIC
%-fpic.o: %.cc
	$(CXX) -fPIC $(CXXFLAGS) -c -o $@ $<

$(TARGET_A): $(LIB_OBJECTS)
	ar rcvu $@ $^

sample: sample.o $(TARGET_A)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -lopencv_imgproc -o $@


ifdef DESTDIR

install: all $(TARGET_SO)
	mkdir -p $(DESTDIR)/usr/lib/
	install -m 0644 $(TARGET_A) $(TARGET_SO) $(DESTDIR)/usr/lib/
	cd $(DESTDIR)/usr/lib/ && \
	ln -fs $(TARGET_SO) libvisionio.so.$(API_VERSION) && \
	ln -fs $(TARGET_SO) libvisionio.so && \
	cd -
	mkdir -p $(DESTDIR)/usr/include/
	install -m 0644 *.hh $(DESTDIR)/usr/include/
else
install:
	@echo "make install is here ONLY for the debian package. Do NOT run it yourself" && false
endif


clean:
	rm -f *.o *.a *.so* *.d sample

.PHONY: all

-include *.d
