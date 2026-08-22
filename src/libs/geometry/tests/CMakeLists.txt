find_package(GTest REQUIRED)

add_executable(antwika_geometry_tests
    GridCellTest.cpp
    GridTest.cpp
    PointFTest.cpp
    RectFTest.cpp
    SizeFTest.cpp
    SizeTest.cpp
)

target_link_libraries(antwika_geometry_tests
    PRIVATE
        antwika::geometry
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_geometry_tests)
