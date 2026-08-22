find_package(glm REQUIRED)

antwika_add_library(
    NAME voxel
    SOURCES
        src/VoxelClimb.cpp
        src/VoxelCube.cpp
        src/VoxelMaterial.cpp
        src/VoxelOcclusion.cpp
        src/VoxelStairs.cpp
    DEPENDS
        antwika::enums
        glm::glm
)
