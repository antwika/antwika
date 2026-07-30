find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(mocks)

add_executable(antwika_gfx_tests
    ColorTest.cpp
    GlyphsTest.cpp
    NullBackendConformanceTest.cpp
    NullBackendTest.cpp
    NullRendererTest.cpp
    NullWindowTest.cpp
    PointTest.cpp
    RectTest.cpp
    SizeTest.cpp
    TextLayoutTest.cpp
    WindowIdTest.cpp
)

target_include_directories(antwika_gfx_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_gfx_tests
    PRIVATE
        antwika::gfx
        antwika::gfx::tests::conformance
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_gfx_tests)
