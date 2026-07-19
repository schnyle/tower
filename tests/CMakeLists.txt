include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(tower_tests
  test.cpp
)
target_link_libraries(tower_tests PRIVATE tower_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tower_tests)
