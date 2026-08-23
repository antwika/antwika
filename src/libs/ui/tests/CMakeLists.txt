find_package(GTest REQUIRED)

add_subdirectory(support)

add_executable(antwika_ui_tests
    BuildDrawListTest.cpp
    CheckboxTest.cpp
    ContainerScopeTest.cpp
    ContainsTest.cpp
    ContextHoverTest.cpp
    ContextInteractionTest.cpp
    ContextKeyboardTest.cpp
    ContextNestingTest.cpp
    ContextTest.cpp
    ContextWidgetsTest.cpp
    DoubleClickTest.cpp
    DrawCommandTest.cpp
    DropdownTest.cpp
    FieldValueTest.cpp
    HoverHintTest.cpp
    HoverTest.cpp
    InteractionsTest.cpp
    LayoutDegenerateTest.cpp
    LayoutTest.cpp
    LayoutTreeTest.cpp
    OccluderRectsTest.cpp
    PainterTest.cpp
    ResolveFocusTest.cpp
    ResolveHitTest.cpp
    ResolveTest.cpp
    SizingTest.cpp
    SliderTest.cpp
    SplitTest.cpp
    TextAreaBandTest.cpp
    TextAreaHighlightTest.cpp
    TextAreaScrollTest.cpp
    TextAreaSelectionTest.cpp
    TextAreaTest.cpp
    TextFieldTest.cpp
    TextWrapTest.cpp
    ThemeTest.cpp
    UiErrorTest.cpp
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
        antwika::ui::tests::support
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_ui_tests)
