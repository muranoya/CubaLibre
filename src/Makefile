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

$(TARGET): main.o hashmap.o util.o
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^ $(LDFLAGS)

hashmap: hashmap.c hashmap.h util.o
	$(CC) $(CFLAGS) $(DEBUG) -DTEST_HASHMAP -o $@ $^

clean:
	rm -f *.o $(TARGET) hashmap

