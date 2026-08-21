find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(fakes)
add_subdirectory(mocks)

add_executable(antwika_gfx_tests
    AtlasTextTest.cpp
    BitmapTest.cpp
    BlitTest.cpp
    Camera3DTest.cpp
    ColorSpaceTest.cpp
    ColorTest.cpp
    CubeFaceTest.cpp
    GlyphAtlasTest.cpp
    GlyphAtlasTexturesTest.cpp
    GlyphBlitTest.cpp
    GlyphCellsTest.cpp
    GlyphMetricsTest.cpp
    IWindowTest.cpp
    Math3DTest.cpp
    MeshDataTest.cpp
    MeshMaterialTest.cpp
    NullBackendConformanceTest.cpp
    NullBackendTest.cpp
    NullRenderer3DTest.cpp
    NullRendererTest.cpp
    NullTextureTest.cpp
    NullWindowTest.cpp
    PngFileTest.cpp
    PngReaderTest.cpp
    PngWriterTest.cpp
    PointTest.cpp
    RectTest.cpp
    ShaderReaderTest.cpp
    ShaderSourceTest.cpp
    SizeTest.cpp
    TextLayoutTest.cpp
    TextRasterTest.cpp
    TransformTest.cpp
    ViewportRendererTest.cpp
    ViewportTest.cpp
    WindowIdTest.cpp
    WindowSpecTest.cpp
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
        antwika::io
        antwika::log::tests::mocks
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_gfx_tests)
