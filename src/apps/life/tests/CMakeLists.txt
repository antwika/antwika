find_package(GTest REQUIRED)

add_executable(antwika_life_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/BoardSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Life.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LifeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PrintSystem.cpp
    BoardSinkTest.cpp
    BoardTest.cpp
    BootstrapTest.cpp
    GridTest.cpp
    LifeSystemTest.cpp
    PrintSystemTest.cpp
    ReplayIntegrationTest.cpp
)

target_include_directories(antwika_life_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_life_tests
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_life_tests)
