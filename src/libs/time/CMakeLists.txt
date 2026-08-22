cmake_minimum_required(VERSION 3.28)

project(antwika-time VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ANTWIKA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(ANTWIKA_CCACHE ccache)

    if(ANTWIKA_CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER "${ANTWIKA_CCACHE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${ANTWIKA_CCACHE}")
    endif()
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

if(ENABLE_COVERAGE AND NOT CMAKE_CROSSCOMPILING)
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

add_library(antwika_time
    src/FrameRate.cpp
    src/RemainingBudget.cpp
    src/SystemClock.cpp
    src/ThreadSleeper.cpp
)

set_target_properties(antwika_time PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_time
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(
    TARGETS antwika_time
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)
