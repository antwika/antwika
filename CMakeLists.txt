cmake_minimum_required(VERSION 3.28)

project(antwika CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(AntwikaModule)
include(CTest)

enable_testing()

option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ANTWIKA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

option(ANTWIKA_BUILD_BENCHMARKS
    "Build the benchmark executables under benchmarks/" OFF)

set(ANTWIKA_GFX_BACKEND "null" CACHE STRING
    "Graphics backend to compile and link, named after a backends/ subdirectory")

set(ANTWIKA_INPUT_BACKEND "auto" CACHE STRING
    "Input backend named after a backends/ subdirectory, or auto to follow ANTWIKA_GFX_BACKEND")

if(ANTWIKA_INPUT_BACKEND STREQUAL "auto")
    set(ANTWIKA_INPUT_BACKEND "${ANTWIKA_GFX_BACKEND}")
endif()

set(ANTWIKA_SOUND_BACKEND "null" CACHE STRING
    "Sound backend to compile and link, named after a backends/ subdirectory")

set(ANTWIKA_NETWORK_BACKEND "null" CACHE STRING
    "Network backend to compile and link, named after a backends/ subdirectory")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wsuggest-override
    )

    if(ANTWIKA_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

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

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)

    file(WRITE "${CMAKE_BINARY_DIR}/antwika_test_pch.cpp" "")

    add_library(antwika_test_pch OBJECT
        "${CMAKE_BINARY_DIR}/antwika_test_pch.cpp")

    target_link_libraries(antwika_test_pch PRIVATE gtest::gtest)

    target_precompile_headers(antwika_test_pch
        PRIVATE
            <gmock/gmock.h>
            <gtest/gtest.h>
    )
endif()

add_subdirectory(src)
add_subdirectory(backends)

if(ANTWIKA_BUILD_BENCHMARKS AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(benchmarks)
endif()

antwika_install_notices(share/doc/antwika)

function(antwika_collect_executables directory out_variable)
    get_property(targets DIRECTORY "${directory}"
        PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirectories DIRECTORY "${directory}"
        PROPERTY SUBDIRECTORIES)

    set(collected "")

    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)

        if(type STREQUAL "EXECUTABLE")
            list(APPEND collected ${target})
        endif()
    endforeach()

    foreach(subdirectory IN LISTS subdirectories)
        antwika_collect_executables("${subdirectory}" nested)
        list(APPEND collected ${nested})
    endforeach()

    set(${out_variable} "${collected}" PARENT_SCOPE)
endfunction()

antwika_collect_executables("${CMAKE_CURRENT_SOURCE_DIR}"
    ANTWIKA_EXECUTABLE_TARGETS)

list(SORT ANTWIKA_EXECUTABLE_TARGETS)

foreach(target IN LISTS ANTWIKA_EXECUTABLE_TARGETS)
    if(TARGET antwika_test_pch AND target MATCHES "_tests$")
        target_precompile_headers(${target} REUSE_FROM antwika_test_pch)

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target}
                PRIVATE -Wno-error=invalid-pch)
        endif()

        get_target_property(target_directory ${target} SOURCE_DIR)
        get_target_property(target_sources ${target} SOURCES)

        foreach(source IN LISTS target_sources)
            get_filename_component(resolved "${source}" REALPATH
                BASE_DIR "${target_directory}")

            string(FIND "${resolved}" "${target_directory}/" inside)

            if(NOT inside EQUAL 0)
                set_source_files_properties("${source}"
                    TARGET_DIRECTORY ${target}
                    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
            endif()
        endforeach()
    endif()
endforeach()

set(ANTWIKA_EXECUTABLE_PATHS "")

foreach(target IN LISTS ANTWIKA_EXECUTABLE_TARGETS)
    list(APPEND ANTWIKA_EXECUTABLE_PATHS "$<TARGET_FILE:${target}>")
endforeach()

list(JOIN ANTWIKA_EXECUTABLE_PATHS "\n" ANTWIKA_EXECUTABLE_MANIFEST)

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/expected-executables.txt"
    CONTENT "${ANTWIKA_EXECUTABLE_MANIFEST}\n"
)
