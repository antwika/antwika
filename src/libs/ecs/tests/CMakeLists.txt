find_package(GTest REQUIRED)

add_subdirectory(fakes)
add_subdirectory(mocks)

add_executable(antwika_ecs_tests
    ComponentKeyTest.cpp
    ComponentStorageTest.cpp
    ComponentTest.cpp
    DoubleBufferingTest.cpp
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
        antwika::ecs::tests::fakes
        antwika::ecs::tests::mocks
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_ecs_tests)
