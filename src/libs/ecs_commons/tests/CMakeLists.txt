find_package(GTest REQUIRED)

# The sources are compiled in rather than linked from the library.
# Linking it would put a second copy of every antwika::ecs template
# this library instantiates into the binary, and the linker keeps one.
# The discarded copy's counters then read zero however much runs.
add_executable(antwika_ecs_commons_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LifetimeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/MovementSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PeriodicSystem.cpp
    ComponentsTest.cpp
    LifetimeSystemTest.cpp
    MovementSystemTest.cpp
    NameTest.cpp
    PeriodicSystemTest.cpp
)

target_include_directories(antwika_ecs_commons_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_ecs_commons_tests
    PRIVATE
        antwika::ecs
        antwika::time
        antwika::ecs::tests::mocks
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ecs_commons_tests)
