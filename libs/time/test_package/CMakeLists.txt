cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(time CONFIG REQUIRED)

find_package(GTest REQUIRED)

add_executable(time_test
    src/antwika/time/FakeClockTest.cpp
    src/antwika/time/SystemClockTest.cpp
)

target_link_libraries(time_test antwika::time)

target_link_libraries(time_test gtest::gtest)
