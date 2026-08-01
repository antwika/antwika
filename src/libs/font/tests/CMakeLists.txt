find_package(GTest REQUIRED)

# SyntheticFont is a fixture builder rather than a test double, so it
# sits here rather than under mocks/ or fakes/: what it stands in for
# is a font binary nobody wants in the repository, not a collaborator.
add_executable(antwika_font_tests
    CoverageTest.cpp
    FontMetricsTest.cpp
    FontTest.cpp
    GlyphAtlasTest.cpp
    GlyphMetricsTest.cpp
    GlyphTest.cpp
    RectTest.cpp
    SyntheticFont.cpp
    FontErrorTest.cpp
    TtfReaderTest.cpp
)

target_link_libraries(antwika_font_tests
    PRIVATE
        antwika::font
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_font_tests)
