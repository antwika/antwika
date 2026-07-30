find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(mocks)

add_executable(antwika_gfx_tests
    BitmapTest.cpp
    BlitTest.cpp
    Camera3DTest.cpp
    ColorTest.cpp
    GlyphsTest.cpp
    IRendererTest.cpp
    IWindowTest.cpp
    Math3DTest.cpp
    MeshDataTest.cpp
    NullBackendConformanceTest.cpp
    NullBackendTest.cpp
    NullRenderer3DTest.cpp
    NullRendererTest.cpp
    NullTextureTest.cpp
    NullWindowTest.cpp
    PngReaderTest.cpp
    PointTest.cpp
    RectTest.cpp
    SizeTest.cpp
    TextLayoutTest.cpp
    TextRasterTest.cpp
    TransformTest.cpp
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
        antwika::gfx::tests::mocks
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_gfx_tests)
