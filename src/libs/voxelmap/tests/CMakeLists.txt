find_package(GTest REQUIRED)

add_executable(antwika_voxelmap_tests
    VoxelMeshPiecesTest.cpp
    VoxelPickTest.cpp
    VoxelTest.cpp
)

target_include_directories(antwika_voxelmap_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_voxelmap_tests
    PRIVATE
        antwika::camera
        antwika::gfx
        antwika::voxelmap
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_voxelmap_tests)
