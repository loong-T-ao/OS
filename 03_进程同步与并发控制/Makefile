CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -pthread
TARGET = bin/sync_demo
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) -m all -c ./tests/sync_config.txt

clean:
	rm -rf bin
