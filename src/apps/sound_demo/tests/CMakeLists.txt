antwika_add_app_tests(
    APP sound_demo
    TESTS
        DemoLoopTest.cpp
        DemoOptionsTest.cpp
        DemoTrackTest.cpp
    LIBS
        antwika::cli
        antwika::log::tests::mocks
        antwika::sound
        antwika::time
        antwika::time::tests::fakes
        gtest::gtest
)
