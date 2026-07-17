.PHONY: build
build:
	g++ -std=c++20 -Iinclude src/main.cpp src/tui.cpp src/loadavg.cpp src/meminfo.cpp -o build/tower
