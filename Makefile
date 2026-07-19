.PHONY: setup build test test-output

setup:
	cmake -B build

build: setup
	cmake --build build

test: build
	ctest --test-dir build --output-on-failure

