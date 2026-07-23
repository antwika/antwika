cmake_minimum_required(VERSION 3.28)

project(log CXX)

if(NOT TARGET antwika::time)
    find_package(time REQUIRED)
endif()

add_library(log
    src/antwika/log/Logger.cpp
    src/antwika/log/NullAppender.cpp
    src/antwika/log/StreamAppender.cpp
)

add_library(antwika::log ALIAS log)

set(LOG_PUBLIC_HEADERS
    include/antwika/log/Appender.hpp
    include/antwika/log/Logger.hpp
    include/antwika/log/NullAppender.hpp
    include/antwika/log/StreamAppender.hpp
)

set_target_properties(log PROPERTIES
    PUBLIC_HEADER "${LOG_PUBLIC_HEADERS}"
)

target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(log PUBLIC antwika::time)

add_subdirectory(mocks)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(
    TARGETS log
    EXPORT logTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT logTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/log
)
