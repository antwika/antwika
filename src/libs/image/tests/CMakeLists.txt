find_package(GTest REQUIRED)

add_executable(antwika_image_tests
    PngFileTest.cpp
    PngReaderTest.cpp
    PngWriterTest.cpp
)

target_link_libraries(antwika_image_tests
    PRIVATE
        antwika::image
        antwika::io
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_image_tests)
