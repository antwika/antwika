find_package(GTest REQUIRED)

add_executable(antwika_animation_tests
    AnimationErrorTest.cpp
    ClipTest.cpp
    FrameTest.cpp
    KeyFrameTest.cpp
    PlaybackTest.cpp
    ProgressTest.cpp
)

target_link_libraries(antwika_animation_tests
    PRIVATE
        antwika::animation
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_animation_tests)
