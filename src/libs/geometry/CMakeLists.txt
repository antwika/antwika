add_library(antwika_geometry INTERFACE)

add_library(antwika::geometry ALIAS antwika_geometry)

target_include_directories(antwika_geometry
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_geometry
    EXPORT antwika_geometryTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_geometryTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
