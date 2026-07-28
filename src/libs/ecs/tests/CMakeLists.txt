find_package(GTest REQUIRED)

add_executable(antwika_ecs_tests
    EntityManagerTest.cpp
)

target_include_directories(antwika_ecs_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_ecs_tests
    PRIVATE
        antwika::ecs
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ecs_tests)
