antwika_add_library(
    NAME tile
    SOURCES
        src/TilePaint.cpp
        src/TileRules.cpp
        src/TileShapes.cpp
        src/Transitions.cpp
    DEPENDS
        antwika::enums
        antwika::geometry
        antwika::gfx
        antwika::tilemap
        antwika::ui
        antwika::voxel
        antwika::voxelmap
)
