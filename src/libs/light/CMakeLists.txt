antwika_add_library(
    NAME light
    SOURCES
        src/ActiveLight.cpp
        src/PointLight.cpp
    DEPENDS
        antwika::component
        antwika::ecs
        antwika::gfx
        antwika::voxel
        antwika::voxelmap
        antwika::collision
)
