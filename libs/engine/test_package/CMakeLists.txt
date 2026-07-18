cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(engine CONFIG REQUIRED)

find_package(log REQUIRED)
find_package(GTest REQUIRED)

add_executable(engine_test
    src/antwika/engine/EngineTest.cpp
)

target_link_libraries(engine_test antwika::engine)

target_link_libraries(engine_test antwika::log)

target_link_libraries(engine_test gtest::gtest)
