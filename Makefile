CC := cc
VERSION := $(shell tr -d '\n' < VERSION)
CFLAGS := -std=c17 -Wall -Wextra -Werror -Wpedantic -Iinclude -DLINUX_DOCTOR_VERSION=\"$(VERSION)\"
BUILD_DIR := build
SOURCES := src/main.c src/storage.c src/json.c src/report.c src/history.c src/updates.c src/apps.c src/steam.c src/volume.c src/migration.c src/gfn.c
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
	$(CC) $(CFLAGS) tests/test_history.c src/history.c -o $(BUILD_DIR)/test_history
	$(BUILD_DIR)/test_history
	$(CC) $(CFLAGS) tests/test_updates.c src/updates.c -o $(BUILD_DIR)/test_updates
	$(BUILD_DIR)/test_updates
	$(CC) $(CFLAGS) tests/test_apps.c src/apps.c -o $(BUILD_DIR)/test_apps
	$(BUILD_DIR)/test_apps
	$(CC) $(CFLAGS) tests/test_steam.c src/steam.c -o $(BUILD_DIR)/test_steam
	$(BUILD_DIR)/test_steam
	$(CC) $(CFLAGS) tests/test_volume.c src/volume.c -o $(BUILD_DIR)/test_volume
	$(BUILD_DIR)/test_volume
	$(CC) $(CFLAGS) tests/test_migration.c src/migration.c -o $(BUILD_DIR)/test_migration
	$(BUILD_DIR)/test_migration
	$(CC) $(CFLAGS) tests/test_gfn.c src/gfn.c -o $(BUILD_DIR)/test_gfn
	$(BUILD_DIR)/test_gfn
	$(CC) $(CFLAGS) tests/test_report.c src/report.c src/storage.c src/json.c src/history.c src/updates.c src/apps.c src/steam.c src/volume.c src/migration.c src/gfn.c -o $(BUILD_DIR)/test_report
	$(BUILD_DIR)/test_report

run: $(TARGET)
	$(TARGET) --history --output frontend/report.json

clean:
	rm -rf $(BUILD_DIR)
