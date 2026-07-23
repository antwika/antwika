cmake_minimum_required(VERSION 3.28)
project(PackageTest CXX)

if(NOT TARGET antwika::log)
    find_package(log CONFIG REQUIRED)
endif()

if(NOT TARGET antwika::time)
    find_package(time REQUIRED)
endif()

find_package(GTest REQUIRED)

add_executable(log_test
    src/antwika/log/LoggerTest.cpp
    src/antwika/log/StreamAppenderTest.cpp
)

target_link_libraries(log_test antwika::log)

target_link_libraries(log_test antwika::time)

target_link_libraries(log_test gtest::gtest)
