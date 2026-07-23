cmake_minimum_required(VERSION 3.15)

project(engine CXX)

find_package(time REQUIRED)
find_package(log REQUIRED)

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
