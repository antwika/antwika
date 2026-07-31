find_package(GTest REQUIRED)

add_executable(antwika_ecs_commons_tests
    ComponentsTest.cpp
    LifetimeSystemTest.cpp
    MovementSystemTest.cpp
    NameTest.cpp
    PeriodicSystemTest.cpp
)

target_link_libraries(antwika_ecs_commons_tests
    PRIVATE
        antwika::ecs_commons
        antwika::ecs::tests::mocks
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ecs_commons_tests)
