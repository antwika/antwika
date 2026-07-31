find_package(GTest REQUIRED)

add_executable(antwika_tower_defence_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LevelTile.cpp
    LevelGeneratorTest.cpp
    LevelTileTest.cpp
)

target_include_directories(antwika_tower_defence_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_tower_defence_tests
    PRIVATE
        antwika::wfc
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_tower_defence_tests)
