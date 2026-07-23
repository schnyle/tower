cmake_minimum_required(VERSION 3.20)
project(tower CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(tower_lib STATIC
  src/tui.cpp
  src/screen_buffer.cpp
  src/loadavg.cpp
  src/meminfo.cpp
  src/stat.cpp
)
target_include_directories(tower_lib PUBLIC include)
target_compile_options(tower_lib PUBLIC -Wall -Wextra -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=)

add_executable(tower src/main.cpp)
target_link_libraries(tower PRIVATE tower_lib)

enable_testing()
add_subdirectory(tests)

