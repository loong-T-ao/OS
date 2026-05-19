CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = bin/scheduler
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) -i ./tests/sample_processes.txt -a all -q 2

clean:
	rm -rf bin
