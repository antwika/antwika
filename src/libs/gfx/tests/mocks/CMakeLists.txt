add_library(antwika_gfx_tests_mocks INTERFACE)

add_library(antwika::gfx::tests::mocks ALIAS antwika_gfx_tests_mocks)

target_include_directories(antwika_gfx_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_gfx_tests_mocks
    INTERFACE
        antwika::gfx
)

install(
    TARGETS antwika_gfx_tests_mocks
    EXPORT antwika_gfxTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
