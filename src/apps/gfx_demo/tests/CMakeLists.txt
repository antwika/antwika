antwika_add_app_tests(
    APP gfx_demo
    TESTS
        DemoLoopTest.cpp
        DemoSceneTest.cpp
    LIBS
        antwika::app
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::input::tests::fakes
        antwika::time::tests::fakes
        antwika::ui
        gtest::gtest
)
