CC := cc
CFLAGS := -std=c17 -Wall -Wextra -Werror -Wpedantic -Iinclude
BUILD_DIR := build
SOURCES := src/main.c src/storage.c src/json.c src/report.c
TARGET := $(BUILD_DIR)/linux-doctor

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@

test: | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_storage.c src/storage.c -o $(BUILD_DIR)/test_storage
	$(BUILD_DIR)/test_storage
	$(CC) $(CFLAGS) tests/test_json.c src/json.c -o $(BUILD_DIR)/test_json
	$(BUILD_DIR)/test_json

run: $(TARGET)
	$(TARGET) --output frontend/report.json

clean:
	rm -rf $(BUILD_DIR)
