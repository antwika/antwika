cmake_minimum_required(VERSION 3.28)

project(antwika CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(CTest)

enable_testing()

option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ANTWIKA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

set(ANTWIKA_GFX_BACKEND "null" CACHE STRING
    "Graphics backend to compile and link, named after a backends/ subdirectory")

# Defaults to the graphics choice, so one flag drives both in the common
# case: the default build stays null/null and depends on nothing new,
# while asking for sdl3 windows gets sdl3 input for free.
# Setting the two apart is still legal -- input with no window, or a
# window with no input -- as long as they do not name two real
# frameworks; backends/CMakeLists.txt is where that is enforced, and why.
set(ANTWIKA_INPUT_BACKEND "${ANTWIKA_GFX_BACKEND}" CACHE STRING
    "Input backend to compile and link, named after a backends/ subdirectory")

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

set(ANTWIKA_EXECUTABLE_PATHS "")

foreach(target IN LISTS ANTWIKA_EXECUTABLE_TARGETS)
    list(APPEND ANTWIKA_EXECUTABLE_PATHS "$<TARGET_FILE:${target}>")
endforeach()

list(JOIN ANTWIKA_EXECUTABLE_PATHS "\n" ANTWIKA_EXECUTABLE_MANIFEST)

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/expected-executables.txt"
    CONTENT "${ANTWIKA_EXECUTABLE_MANIFEST}\n"
)
