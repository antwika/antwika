find_package(GTest REQUIRED)

# Losing the list would build a test binary holding only the tests.
# Every one of them would still pass, over a library it never compiled.
if(NOT ANTWIKA_ECS_COMMONS_SOURCES)
    message(FATAL_ERROR
        "antwika_ecs_commons_tests: ANTWIKA_ECS_COMMONS_SOURCES is "
        "empty; it is set in ../CMakeLists.txt and compiled in here")
endif()

# The sources are compiled in rather than linked from the library.
# Linking it would put a second copy of every antwika::ecs template
# this library instantiates into the binary, and the linker keeps one.
# The discarded copy's counters then read zero however much runs.
# The list itself is the library's own, named in ../CMakeLists.txt, so
# a source can never reach one of the two targets and not the other.
add_executable(antwika_ecs_commons_tests
    ${ANTWIKA_ECS_COMMONS_SOURCES}
    ComponentsTest.cpp
    GatedSystemTest.cpp
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

antwika_bundle_test(TARGET antwika_ecs_commons_tests)
