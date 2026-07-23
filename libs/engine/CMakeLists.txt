cmake_minimum_required(VERSION 3.28)

project(engine CXX)

if(NOT TARGET antwika::time)
    find_package(time REQUIRED)
endif()

if(NOT TARGET antwika::log)
    find_package(log REQUIRED)
endif()

add_library(engine src/antwika/engine/Engine.cpp)

add_library(antwika::engine ALIAS engine)

set(ENGINE_PUBLIC_HEADERS
    include/antwika/engine/Engine.hpp
)

set_target_properties(engine PROPERTIES
    PUBLIC_HEADER "${ENGINE_PUBLIC_HEADERS}"
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine PRIVATE antwika::time)
target_link_libraries(engine PRIVATE antwika::log)

add_subdirectory(mocks)


if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(
    TARGETS engine
    EXPORT engineTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT engineTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/engine
)
