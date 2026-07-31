# No DEPENDS, and that is the point: parsing a flag needs nothing from
# this project, so a program wanting one does not link a replay format
# to get it.
antwika_add_library(
    NAME cli
    SOURCES
        src/CommandLine.cpp
)
