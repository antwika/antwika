find_package(GTest REQUIRED)

add_executable(antwika_text_tests
    AtlasTextTest.cpp
    GlyphAtlasTest.cpp
    GlyphAtlasTexturesTest.cpp
    GlyphBlitTest.cpp
    GlyphCellsTest.cpp
    GlyphMetricsTest.cpp
    TextLayoutTest.cpp
    TextRasterTest.cpp
)

target_include_directories(antwika_text_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_text_tests
    PRIVATE
        antwika::font
        antwika::gfx::tests::mocks
        antwika::text
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_text_tests)
