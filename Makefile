CC = gcc
CFLAGS = -Wall -Wextra -I include -I unity

SRC = src/main.c src/config.c src/metric.c src/tui/tui.c src/tui/rect.c
TARGET = bin/tower

TEST_SRC = src/ring_buffer.c unity/unity.c tests/test_ring_buffer.c
TEST_TARGET = bin/test_runner

$(TARGET): $(SRC) 
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f bin/tower
	rm -f bin/test_runner

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

test: $(TEST_SRC) 
	$(CC) $(CFLAGS) $^ -o bin/test_runner
	./bin/test_runner
