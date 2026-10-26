# make test      host unit tests of the lock state machine (g++)
# make compile   build the ESP32 firmware with arduino-cli -> build/
# make upload PORT=/dev/ttyUSB0
ARDUINO_CLI ?= arduino-cli
FQBN        ?= esp32:esp32:esp32
SKETCH      := firmware/door_lock
PORT        ?= /dev/ttyUSB0

.PHONY: test compile upload clean

test: build/test_lock_logic
	./build/test_lock_logic

build/test_lock_logic: test/test_lock_logic.cpp $(SKETCH)/lock_logic.h
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -O2 -o $@ $<

compile:
	$(ARDUINO_CLI) compile --fqbn $(FQBN) --output-dir build $(SKETCH)

upload: compile
	$(ARDUINO_CLI) upload --fqbn $(FQBN) -p $(PORT) --input-dir build $(SKETCH)

clean:
	rm -rf build
