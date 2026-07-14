CC := cc
VERSION := $(shell tr -d '\n' < VERSION)
CFLAGS := -std=c17 -Wall -Wextra -Werror -Wpedantic -Iinclude -DLINUX_DOCTOR_VERSION=\"$(VERSION)\"
LDLIBS := -ldl
BUILD_DIR := build
SOURCES := src/main.c src/storage.c src/json.c src/report.c src/history.c src/updates.c src/process.c src/apps.c src/steam.c src/volume.c src/migration.c src/gfn.c src/graphics.c src/knowledge.c src/future_lab.c src/future_lab_json.c
TARGET := $(BUILD_DIR)/linux-doctor

.PHONY: all clean test test-frontend run

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

test: $(TARGET) | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_storage.c src/storage.c -o $(BUILD_DIR)/test_storage
	$(BUILD_DIR)/test_storage
	$(CC) $(CFLAGS) tests/test_json.c src/json.c -o $(BUILD_DIR)/test_json
	$(BUILD_DIR)/test_json
	$(CC) $(CFLAGS) tests/test_history.c src/history.c -o $(BUILD_DIR)/test_history
	$(BUILD_DIR)/test_history
	$(CC) $(CFLAGS) tests/test_process.c src/process.c -o $(BUILD_DIR)/test_process
	$(BUILD_DIR)/test_process
	$(CC) $(CFLAGS) tests/test_updates.c src/updates.c src/process.c -o $(BUILD_DIR)/test_updates
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
	$(CC) $(CFLAGS) tests/test_graphics.c src/graphics.c -o $(BUILD_DIR)/test_graphics $(LDLIBS)
	$(BUILD_DIR)/test_graphics
	$(CC) $(CFLAGS) tests/test_knowledge.c src/knowledge.c -o $(BUILD_DIR)/test_knowledge
	$(BUILD_DIR)/test_knowledge
	$(CC) $(CFLAGS) tests/test_future_lab.c src/future_lab.c -o $(BUILD_DIR)/test_future_lab
	$(BUILD_DIR)/test_future_lab
	$(CC) $(CFLAGS) tests/test_future_lab_json.c src/future_lab_json.c src/future_lab.c src/json.c -o $(BUILD_DIR)/test_future_lab_json
	$(BUILD_DIR)/test_future_lab_json
	$(CC) $(CFLAGS) tests/test_report.c src/report.c src/storage.c src/json.c src/history.c src/updates.c src/process.c src/apps.c src/steam.c src/volume.c src/migration.c src/gfn.c src/knowledge.c src/future_lab.c src/future_lab_json.c -o $(BUILD_DIR)/test_report
	$(BUILD_DIR)/test_report
	$(TARGET) --future-lab-json > $(BUILD_DIR)/future-lab-live.json
	grep -Fq '"name":"linux-doctor.future-lab.live"' $(BUILD_DIR)/future-lab-live.json

test-frontend:
	node --check frontend/app.js
	node --check frontend/future-lab-core.js
	node --check frontend/future-lab.js
	node --check frontend/future-lab-ai.js
	node --check frontend/future-lab-ai-worker.js
	node --test tests/test_future_lab_frontend.mjs

run: $(TARGET)
	$(TARGET) --history --output frontend/report.json

clean:
	rm -rf $(BUILD_DIR)
