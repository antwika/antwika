find_package(GTest REQUIRED)

add_executable(antwika_ui_demo_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoOverlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/KeyMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TickBudgetSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/UiDemo.cpp
    DemoSceneTest.cpp
    DemoSinkTest.cpp
    DemoStateTest.cpp
    KeyMappingTest.cpp
    RenderSinkTest.cpp
    RunIntegrationTest.cpp
    TickBudgetSourceTest.cpp
)

target_include_directories(antwika_ui_demo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_ui_demo_tests
    PRIVATE
        antwika::app
        antwika::engine
        antwika::event
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        antwika::ui
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ui_demo_tests)
