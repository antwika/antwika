# Not on antwika_add_library: reducer is header-only, so it has no
# sources to compile, no archive to install, and an INTERFACE include
# directory rather than a PUBLIC one.
# It is the only library in the tree shaped that way.
add_library(antwika_reducer INTERFACE)

add_library(antwika::reducer ALIAS antwika_reducer)

target_include_directories(antwika_reducer
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_reducer
    INTERFACE
        antwika::event
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_reducer
    EXPORT antwika_reducerTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_reducerTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
