cmake_minimum_required(VERSION 3.28)
project(PackageTest CXX)

if(NOT TARGET antwika::time)
    find_package(time CONFIG REQUIRED)
endif()

find_package(GTest REQUIRED)

add_executable(time_test
    src/antwika/time/FakeClockTest.cpp
    src/antwika/time/SystemClockTest.cpp
)

target_link_libraries(time_test antwika::time)

target_link_libraries(time_test gtest::gtest)
