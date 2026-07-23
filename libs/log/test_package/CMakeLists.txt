cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(log CONFIG REQUIRED)

find_package(time REQUIRED)
find_package(GTest REQUIRED)

add_executable(log_test
    src/antwika/log/LoggerTest.cpp
    src/antwika/log/StreamAppenderTest.cpp
)

target_link_libraries(log_test antwika::log)

target_link_libraries(log_test antwika::time)

target_link_libraries(log_test gtest::gtest)
