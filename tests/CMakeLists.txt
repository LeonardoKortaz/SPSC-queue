include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(spsc_tests
  test_spsc_basic.cpp
  test_spsc_concurrent.cpp
)
target_link_libraries(spsc_tests
  PRIVATE spsc GTest::gtest_main Threads::Threads)
target_compile_options(spsc_tests PRIVATE ${SPSC_WARN})

gtest_discover_tests(spsc_tests)
