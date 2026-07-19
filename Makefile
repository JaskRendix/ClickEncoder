TEST_DIR := test
BUILD_DIR := $(TEST_DIR)/build
SRC_DIR := src
ARDUINO_CLI := arduino-cli
BOARD := esp32:esp32:esp32
EXAMPLES_DIR := examples
EXAMPLES := $(wildcard $(EXAMPLES_DIR)/*/*.ino)

.PHONY: all test clean examples arduino-setup

all: test

test:
	@echo "Running CMake tests..."
	@rm -rf $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && make
	@$(BUILD_DIR)/run_tests

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

format:
	find src test examples -type f -regex ".*\.\(h\|cpp\|ino\)" -print0 | xargs -0 clang-format -i

arduino-setup:
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp32:esp32
	$(ARDUINO_CLI) lib install SmoothThermistor || true
	$(ARDUINO_CLI) lib install LiquidCrystal || true

examples: arduino-setup
	@echo "Compiling Arduino examples with Arduino CLI..."
	@for d in $(EXAMPLES); do \
		echo "Compiling $$d"; \
		$(ARDUINO_CLI) compile --fqbn $(BOARD) \
			--library . \
			$$d || exit 1; \
	done
