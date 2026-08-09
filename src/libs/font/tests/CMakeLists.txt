find_package(GTest REQUIRED)

add_executable(antwika_font_tests
    CoverageTest.cpp
    FontErrorTest.cpp
    FontMetricsTest.cpp
    FontTest.cpp
    GlyphAtlasTest.cpp
    GlyphMetricsTest.cpp
    GlyphTest.cpp
    RectTest.cpp
    SyntheticFont.cpp
    TtfReaderTest.cpp
)

target_link_libraries(antwika_font_tests
    PRIVATE
        antwika::font
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_font_tests)
