antwika_add_library(
    NAME collision
    SOURCES
        src/Collision.cpp
    DEPENDS
        antwika::component
        antwika::geometry
        antwika::pathfinding
        antwika::tilemap
        antwika::voxel
        antwika::voxelmap
)
