antwika_add_library(
    NAME tilemap
    SOURCES
        src/AtlasLayout.cpp
        src/Tilemap.cpp
    DEPENDS
        antwika::geometry
        antwika::gfx
        antwika::voxel
)
