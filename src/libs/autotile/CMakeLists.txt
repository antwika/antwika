antwika_add_library(
    NAME autotile
    SOURCES
        src/Cutaway.cpp
        src/DrawPlan.cpp
        src/SheetLayout.cpp
    DEPENDS
        antwika::geometry
        antwika::tilemap
)
