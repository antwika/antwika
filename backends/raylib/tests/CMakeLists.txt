find_package(GTest REQUIRED)

add_executable(antwika_gfx_backend_raylib_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibWindow.cpp
    RaylibBackendConformanceTest.cpp
    RaylibBackendTest.cpp
)

target_include_directories(antwika_gfx_backend_raylib_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_gfx_backend_raylib_tests
    PRIVATE
        antwika::gfx
        antwika::gfx::tests::conformance
        gtest::gtest
        raylib
)

include(GoogleTest)
gtest_discover_tests(antwika_gfx_backend_raylib_tests)
