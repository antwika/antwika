find_package(GTest REQUIRED)

add_executable(antwika_tween_tests
    EaseTest.cpp
    EasingTest.cpp
    TweenErrorTest.cpp
    TweenTest.cpp
)

target_link_libraries(antwika_tween_tests
    PRIVATE
        antwika::tween
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_tween_tests)
