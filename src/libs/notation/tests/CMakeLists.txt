find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_notation_tests
    NotationErrorTest.cpp
    NumberWordsTest.cpp
    ParsePatternTest.cpp
)

target_link_libraries(antwika_notation_tests
    PRIVATE
        antwika::notation::tests::fakes
        antwika::notation
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_notation_tests)
