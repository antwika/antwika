cmake_minimum_required(VERSION 3.28)

project(antwika CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

enable_testing()

option(ENABLE_COVERAGE "Enable coverage" OFF)

if(ENABLE_COVERAGE AND BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
            -O0
            -g
            --coverage
        )

        add_link_options(
            --coverage
        )
    endif()
endif()

add_subdirectory(libs/time)
add_subdirectory(libs/log)
add_subdirectory(libs/engine)

add_subdirectory(apps/app)
