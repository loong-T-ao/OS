CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = bin/memory_manager
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) -m all -p ./tests/page_input.txt -d ./tests/partition_input.txt

clean:
	rm -rf bin
