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

set(ANTWIKA_GFX_BACKEND "null" CACHE STRING
    "Graphics backend to compile and link, named after a backends/ subdirectory")

# Follows the graphics choice, so one flag drives both in the common
# case: the default build stays null/null and depends on nothing new,
# while asking for sdl3 windows gets sdl3 input for free.
# Setting the two apart is still legal -- input with no window, or a
# window with no input -- as long as they do not name two real
# frameworks; backends/CMakeLists.txt is where that is enforced, and why.
#
# The cache entry holds the sentinel rather than the resolved name,
# because a CACHE default is taken on the first configure only.
# Caching the graphics choice instead left it stale the moment an
# existing build directory was reconfigured with a different
# -DANTWIKA_GFX_BACKEND, and the guard downstream then refused two
# different frameworks without anything pointing at the cache as the
# cause.
# Resolving into a plain variable of the same name shadows the entry for
# every subdirectory, so nothing below has to know about the sentinel.
set(ANTWIKA_INPUT_BACKEND "auto" CACHE STRING
    "Input backend named after a backends/ subdirectory, or auto to follow ANTWIKA_GFX_BACKEND")

if(ANTWIKA_INPUT_BACKEND STREQUAL "auto")
    set(ANTWIKA_INPUT_BACKEND "${ANTWIKA_GFX_BACKEND}")
endif()

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

# One precompiled header, shared by every test target.
# See the loop at the bottom of this file for why it is worth having and
# why it has to be exactly one.
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

# Every executable this project defines, one absolute path per line.
# CI checks that each one is really on disk after a build, and which
# executables exist is knowledge CMake already has -- so it writes the
# list rather than a workflow repeating it, which is what stops a new app
# or test binary from being added without CI ever looking at it.
# The paths come from $<TARGET_FILE>, so a cross build's .exe suffix and
# any future output directory follow along on their own.
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

# <gtest/gtest.h> and <gmock/gmock.h> cost about a second to parse, and
# 273 of this project's 490 translation units are tests -- roughly a
# fifth of a clean build spent on the same two headers.
# Unlike a unity build this moves no symbol between translation units,
# so nothing about what compiles or what a test asserts changes.
#
# REUSE_FROM rather than a header per target, because the 24 test
# targets average eleven translation units each and building a
# precompiled header costs about what eleven of them save.
# Giving each target its own was measured at 24.4 CPU-minutes against
# 24.6 without any -- a wash -- so the saving only exists if the header
# is built once and every target shares it.
#
# It is applied here, from the list of targets CMake already collected,
# rather than by 24 identical edits to the tests/CMakeLists.txt files
# with nothing checking that none was missed -- the same reasoning
# cmake/AntwikaModule.cmake opens with.
foreach(target IN LISTS ANTWIKA_EXECUTABLE_TARGETS)
    if(TARGET antwika_test_pch AND target MATCHES "_tests$")
        target_precompile_headers(${target} REUSE_FROM antwika_test_pch)

        # One header shared by every test target means every test target
        # has to agree with it about the preprocessor, and a target is
        # free to disagree for a reason of its own: SDL3::SDL3 carries
        # -pthread, so backends/sdl3's suites define _REENTRANT and the
        # header -- built against gtest alone -- does not.
        # GCC then declines to use it, which is the correct answer, and
        # -Werror turned that into a failed build.
        # A precompiled header is a build-time optimisation and never a
        # statement about the program, so a target that cannot take this
        # one parses the two headers itself and is merely slower.
        # The warning is left on, since which targets pay full price is
        # worth being able to read off a build log.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target}
                PRIVATE -Wno-error=invalid-pch)
        endif()

        # A test target compiles the application sources it covers, and
        # a precompiled header is force-included into every source of a
        # target rather than into the ones that wanted it.
        # Letting it reach those would put <gmock/gmock.h> ahead of
        # application code that the app itself builds without it, which
        # is a different translation unit from the one that ships -- and
        # it is not theoretical: doing so moved
        # apps/poker/src/TablePrinter.cpp from 255 covered branches to
        # 253, so the coverage gate would have been reporting on code
        # GoogleTest had helped compile.
        # A target's own directory is its tests/, so anything outside it
        # is application source and opts out.
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
