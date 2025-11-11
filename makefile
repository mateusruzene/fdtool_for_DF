CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = fdtool

all: $(TARGET)

$(TARGET): fdtool.c
	$(CC) $(CFLAGS) -o $(TARGET) fdtool.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
