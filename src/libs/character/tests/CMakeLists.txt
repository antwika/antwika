find_package(GTest REQUIRED)

add_executable(antwika_character_tests
    CharacterMarksTest.cpp
    CharacterTest.cpp
)

target_link_libraries(antwika_character_tests
    PRIVATE
        antwika::camera
        antwika::character
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_character_tests)
