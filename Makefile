CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/config.c
TARGET = bin/tower

$(TARGET): $(SRC) 
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f bin/tower

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)
