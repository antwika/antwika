add_library(antwika_component INTERFACE)

add_library(antwika::component ALIAS antwika_component)

target_include_directories(antwika_component
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_component
    INTERFACE
        antwika::enums
        antwika::gfx
        antwika::time
        antwika::voxel
)

install(
    TARGETS antwika_component
    EXPORT antwika_componentTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_componentTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
