find_package(GTest REQUIRED)

add_executable(antwika_animation_tests
    AnimationErrorTest.cpp
    ClipTest.cpp
    DirectionalClipSetTest.cpp
    FacingTest.cpp
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

include(GoogleTest)
gtest_discover_tests(antwika_animation_tests)
