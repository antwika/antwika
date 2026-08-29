antwika_add_library(
    NAME rules
    SOURCES
        src/Health.cpp
        src/Items.cpp
        src/MarkerCubes.cpp
        src/Orientation.cpp
    DEPENDS
        antwika::component
        antwika::tilemap
        antwika::time
        antwika::voxel
)
