antwika_add_library(
    NAME atlas
    SOURCES
        src/AtlasMeta.cpp
        src/AtlasMetaFile.cpp
    DEPENDS
        antwika::enums
        antwika::config
        antwika::gfx
        antwika::replay
)
