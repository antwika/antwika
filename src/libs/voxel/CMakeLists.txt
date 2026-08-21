find_package(glm REQUIRED)

antwika_add_library(
    NAME voxel
    SOURCES
        src/VoxelCell.cpp
        src/VoxelClimb.cpp
        src/VoxelCube.cpp
        src/VoxelOcclusion.cpp
        src/VoxelStairs.cpp
    DEPENDS
        glm::glm
)
