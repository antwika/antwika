antwika_add_library(
    NAME solver
    SOURCES
        src/ChunkGrow.cpp
        src/FaceSeams.cpp
        src/VoxelWeave.cpp
        src/VoxelWeaveDetail.cpp
        src/VoxelWeaveWants.cpp
    DEPENDS
        antwika::enums
        antwika::gfx
        antwika::tilemap
        antwika::tile
        antwika::voxel
        antwika::voxelmap
        antwika::wfc
        antwika::worldgen
)
