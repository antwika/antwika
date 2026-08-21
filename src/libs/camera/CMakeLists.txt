antwika_add_library(
    NAME camera
    SOURCES
        src/FlyCamera.cpp
    DEPENDS
        antwika::gfx
        antwika::tilemap
        antwika::voxel
)
