antwika_add_library(
    NAME assets
    SOURCES
        src/MapAssets.cpp
        src/ShaderAssets.cpp
    DEPENDS
        antwika::character
        antwika::gfx
        antwika::image
        antwika::io
        antwika::map
        antwika::solver
        antwika::tile
        antwika::tilemap
        antwika::voxel
        antwika::voxelmap
)
