CC=cc
CFLAGS=-O2 -Wall -pthread
LDFLAGS=-luv 
DEBUG=
TARGET=cubalibre

.PHONY: all clean
.SUFFIXES: .o .c
%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG) -c $<

all: $(TARGET)

$(TARGET): main.o
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)

