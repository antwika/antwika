antwika_add_app_tests(
    APP gfx_demo
    TESTS
        DemoLoopTest.cpp
        DemoSceneTest.cpp
        LogoDrawTest.cpp
    LIBS
        antwika::app
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::input::tests::fakes
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        antwika::ui
        antwika::ui::tests::support
        gtest::gtest
)
