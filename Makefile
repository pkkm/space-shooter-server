# This file just launches CMake and/or Make.
# NOTE: each command that's on a separate line runs in its own shell.

BUILD_DIR = build
RUN_COMMAND = "./src/space-shooter-server"

MAKE = make
CMAKE = cmake

default:
	mkdir -p "$(BUILD_DIR)" && cd "$(BUILD_DIR)" && $(CMAKE) .. && $(MAKE) --no-print-directory

.PHONY: run
run: default
	cd "$(BUILD_DIR)" && $(RUN_COMMAND)

.PHONY: clean
clean:
	rm -r "$(BUILD_DIR)"
