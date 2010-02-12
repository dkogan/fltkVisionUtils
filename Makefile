CXXFLAGS += -g
LDFLAGS  += -g

LDLIBS += -lfltk -lpthread -lcamwire_1394 -ldc1394_control

test: camera.o main.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm *.o test
