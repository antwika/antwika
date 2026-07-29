add_library(antwika_gfx_tests_conformance INTERFACE)

add_library(antwika::gfx::tests::conformance ALIAS
    antwika_gfx_tests_conformance)

target_include_directories(antwika_gfx_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_gfx_tests_conformance
    INTERFACE
        antwika::gfx
        antwika::log::tests::mocks
)

install(
    TARGETS antwika_gfx_tests_conformance
    EXPORT antwika_gfxTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
