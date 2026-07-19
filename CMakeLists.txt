cmake_minimum_required(VERSION 3.20)
project(tower CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_executable(tower
  src/main.cpp
  src/tui.cpp
  src/loadavg.cpp
  src/meminfo.cpp
  src/stat.cpp
)

target_include_directories(tower PRIVATE include)
target_compile_options(tower PRIVATE -Wall -Wextra -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=)


