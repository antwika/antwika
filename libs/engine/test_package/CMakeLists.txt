cmake_minimum_required(VERSION 3.28)
project(PackageTest CXX)

if(NOT TARGET antwika::engine)
    find_package(engine CONFIG REQUIRED)
endif()

if(NOT TARGET antwika::time)
    find_package(time REQUIRED)
endif()

if(NOT TARGET antwika::log)
    find_package(log REQUIRED)
endif()

find_package(GTest REQUIRED)

add_executable(engine_test
    src/antwika/engine/EngineTest.cpp
)

target_link_libraries(engine_test antwika::engine)

target_link_libraries(engine_test antwika::time)
target_link_libraries(engine_test antwika::log)

target_link_libraries(engine_test gtest::gtest)
