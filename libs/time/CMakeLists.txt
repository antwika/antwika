cmake_minimum_required(VERSION 3.28)

project(time CXX)

add_library(time
    src/antwika/time/SystemClock.cpp
)

add_library(antwika::time ALIAS time)

set(TIME_PUBLIC_HEADERS
    include/antwika/time/IClock.hpp
    include/antwika/time/SystemClock.hpp
)

set_target_properties(time PROPERTIES
    PUBLIC_HEADER "${TIME_PUBLIC_HEADERS}"
)

target_include_directories(time
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(fakes)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(
    TARGETS time
    EXPORT timeTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT timeTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/time
)
