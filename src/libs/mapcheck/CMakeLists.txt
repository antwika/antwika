antwika_add_library(
    NAME mapcheck
    SOURCES
        src/Validate.cpp
    DEPENDS
        antwika::geometry
        antwika::tilemap
)
