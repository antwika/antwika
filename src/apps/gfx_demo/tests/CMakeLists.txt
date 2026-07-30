find_package(GTest REQUIRED)

add_executable(antwika_gfx_demo_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoScene.cpp
    DemoLoopTest.cpp
    DemoSceneTest.cpp
)

target_include_directories(antwika_gfx_demo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_gfx_demo_tests
    PRIVATE
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::input::tests::fakes
        antwika::ui
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_gfx_demo_tests)
