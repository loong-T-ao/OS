CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = bin/fs_simulator
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) -i ./tests/fs_commands.txt

clean:
	rm -rf bin
