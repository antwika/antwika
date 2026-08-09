add_library(antwika_io INTERFACE)

add_library(antwika::io ALIAS antwika_io)

target_include_directories(antwika_io
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_io
    EXPORT antwika_ioTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_ioTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
