find_package(GTest REQUIRED)

add_executable(antwika_autotile_tests
    CutawayTest.cpp
    DrawPlanTest.cpp
    SystemSheetTest.cpp
    TileDrawTest.cpp
)

target_link_libraries(antwika_autotile_tests
    PRIVATE
        antwika::autotile
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_autotile_tests)
