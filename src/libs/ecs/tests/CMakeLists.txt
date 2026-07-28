find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_ecs_tests
    ComponentStorageTest.cpp
    DoubleBufferingTest.cpp
    EcsDeterminismTest.cpp
    EntityManagerTest.cpp
    SystemSchedulerTest.cpp
    ViewTest.cpp
    WorldTest.cpp
)

target_include_directories(antwika_ecs_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_ecs_tests
    PRIVATE
        antwika::ecs
        antwika::ecs::tests::mocks
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_ecs_tests)
