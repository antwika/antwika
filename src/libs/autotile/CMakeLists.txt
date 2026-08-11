antwika_add_library(
    NAME autotile
    SOURCES
        src/Cutaway.cpp
        src/DrawPlan.cpp
        src/MissingArt.cpp
        src/SystemSheet.cpp
    DEPENDS
        antwika::enums
        antwika::geometry
        antwika::tilemap
        antwika::tileset
)
