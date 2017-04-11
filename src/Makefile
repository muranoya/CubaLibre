CXX=c++
CXXFLAGS=-O2 -Wall -std=c++11 -pthread
LDFLAGS=-luv 
DEBUG=
TARGET=cubalibre

.PHONY: all clean
.SUFFIXES: .o .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $<

all: $(TARGET)

$(TARGET): main.o HttpRequest.o Server.o Config.o
	$(CXX) $(CXXFLAGS) $(DEBUG) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)

