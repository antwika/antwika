find_package(GTest REQUIRED)

add_executable(antwika_map_tests
    HistoryTest.cpp
    MapAssetsTest.cpp
    MapFileTest.cpp
)

target_include_directories(antwika_map_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_map_tests
    PRIVATE
        antwika::map
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_map_tests)
