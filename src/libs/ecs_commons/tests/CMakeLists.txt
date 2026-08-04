find_package(GTest REQUIRED)

add_executable(antwika_ecs_commons_tests
    GatedSystemTest.cpp
    NameTest.cpp
)

target_include_directories(antwika_ecs_commons_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_ecs_commons_tests
    PRIVATE
        antwika::ecs
        antwika::ecs::tests::mocks
        antwika::ecs_commons
        antwika::log::tests::mocks
        antwika::time
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_ecs_commons_tests)
