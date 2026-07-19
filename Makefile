CXX = g++
CXXFLAGS = -std=c++20 -Iinclude -Wall -Wextra
SRCS = src/main.cpp src/tui.cpp src/loadavg.cpp src/meminfo.cpp src/stat.cpp

.PHONY: build build-prod
build:
	$(CXX) $(CXXFLAGS) -DDEBUG $(SRCS) -o build/tower

build-prod:
	$(CXX) $(CXXFLAGS) $(SRCS) -o build/tower
