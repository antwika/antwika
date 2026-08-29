find_package(GTest REQUIRED)

add_executable(antwika_collision_tests
    CollisionTest.cpp
    VoxelWalkGraphTest.cpp
)

target_link_libraries(antwika_collision_tests
    PRIVATE
        antwika::collision
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_collision_tests)
