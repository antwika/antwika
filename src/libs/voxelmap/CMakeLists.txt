antwika_add_library(
    NAME voxelmap
    SOURCES
        src/Voxel.cpp
        src/VoxelFaces.cpp
        src/VoxelHide.cpp
        src/VoxelMesh.cpp
        src/VoxelPick.cpp
    DEPENDS
        antwika::gfx
        antwika::tilemap
        antwika::voxel
)
