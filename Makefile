CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -Werror -g -Iinclude

SRC_DIR = code
TEST_DIR = tests
BUILD_DIR = build

SRC = $(SRC_DIR)/sampler.c
TEST_SRC = $(TEST_DIR)/test_sampler.c

OBJ = $(BUILD_DIR)/sampler.o
TEST_OBJ = $(BUILD_DIR)/test_sampler.o

TEST_BIN = $(BUILD_DIR)/test_sampler

.PHONY: all test clean

all: $(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/sampler.o: $(SRC_DIR)/sampler.c include/sampler.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_sampler.o: $(TEST_DIR)/test_sampler.c include/sampler.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)