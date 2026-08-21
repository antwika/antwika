antwika_add_library(
    NAME character
    SOURCES
        src/Character.cpp
        src/CharacterMarks.cpp
    DEPENDS
        antwika::animation
        antwika::component
        antwika::geometry
        antwika::gfx
        antwika::tile
        antwika::time
        antwika::voxel
        antwika::collision
)
