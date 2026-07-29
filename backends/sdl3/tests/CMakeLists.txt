find_package(GTest REQUIRED)

add_executable(antwika_gfx_backend_sdl3_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Window.cpp
    Sdl3BackendConformanceTest.cpp
)

target_include_directories(antwika_gfx_backend_sdl3_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_gfx_backend_sdl3_tests
    PRIVATE
        antwika::gfx
        antwika::gfx::tests::conformance
        gtest::gtest
        SDL3::SDL3
)

include(GoogleTest)
gtest_discover_tests(antwika_gfx_backend_sdl3_tests)
