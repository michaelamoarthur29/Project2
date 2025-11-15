CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pthread

# Object files you actually have
OBJS = main.o BENSCHILLIBOWL.o

# Final executable name
TARGET = bcb

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c BENSCHILLIBOWL.h
	$(CC) $(CFLAGS) -c main.c

BENSCHILLIBOWL.o: BENSCHILLIBOWL.c BENSCHILLIBOWL.h
	$(CC) $(CFLAGS) -c BENSCHILLIBOWL.c

clean:
	rm -f *.o $(TARGET)
