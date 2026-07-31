find_package(GTest REQUIRED)

add_executable(antwika_ui_tests
    ContextHoverTest.cpp
    ContextInteractionTest.cpp
    ContextKeyboardTest.cpp
    ContextNestingTest.cpp
    ContextTest.cpp
    ContextWidgetsTest.cpp
    DrawCommandTest.cpp
    DropdownTest.cpp
    FieldValueTest.cpp
    FlattenTest.cpp
    HoverTest.cpp
    InteractionsTest.cpp
    LayoutDegenerateTest.cpp
    LayoutTest.cpp
    LayoutTreeTest.cpp
    PainterTest.cpp
    ResolveFocusTest.cpp
    ResolveHitTest.cpp
    ResolveTest.cpp
    ScopeTest.cpp
    SizingTest.cpp
    TextFieldTest.cpp
    ThemeTest.cpp
    WidgetIdTest.cpp
    WidgetRectsTest.cpp
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
