find_package(GTest REQUIRED)

add_executable(antwika_ui_tests
    ContextNestingTest.cpp
    ContextTest.cpp
    ContextWidgetsTest.cpp
    DrawCommandTest.cpp
    FlattenTest.cpp
    LayoutDegenerateTest.cpp
    LayoutTest.cpp
    LayoutTreeTest.cpp
    PainterTest.cpp
    ScopeTest.cpp
    SizingTest.cpp
    ThemeTest.cpp
)

target_include_directories(antwika_ui_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_ui_tests
    PRIVATE
        antwika::gfx::tests::mocks
        antwika::ui
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ui_tests)
