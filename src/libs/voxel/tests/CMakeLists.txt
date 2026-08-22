find_package(GTest REQUIRED)

add_executable(antwika_voxel_tests
    VoxelCellTest.cpp
    VoxelCubeTest.cpp
    VoxelMaterialTest.cpp
    VoxelOcclusionTest.cpp
    VoxelPositionTest.cpp
    VoxelStairsTest.cpp
    VoxelsTest.cpp
)

target_link_libraries(antwika_voxel_tests
    PRIVATE
        antwika::voxel
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_voxel_tests)
