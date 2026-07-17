.PHONY: build
build:
	g++ -std=c++20 -Iinclude src/main.cpp src/meminfo.cpp src/tui.cpp -o build/tower
