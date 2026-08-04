antwika_add_app_tests(
    APP gfx3d_demo
    TESTS
        CubeMeshTest.cpp
        SpinLoopTest.cpp
        SpinSceneTest.cpp
    LIBS
        antwika::app
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)
