find_package(GTest REQUIRED)

add_executable(antwika_widget_tests
    WidgetIdTest.cpp
)

target_link_libraries(antwika_widget_tests
    PRIVATE
        antwika::widget
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_widget_tests)
