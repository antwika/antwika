antwika_add_library(
    NAME worldgen
    SOURCES
        src/ChunkShape.cpp
        src/CityRuleset.cpp
        src/Expand.cpp
        src/Grow.cpp
        src/Lattice.cpp
        src/Ruleset.cpp
        src/Stairs.cpp
    DEPENDS
        antwika::enums
        antwika::rng
        antwika::voxel
        antwika::wfc
)
