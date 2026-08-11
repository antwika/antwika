find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME tileset
    SOURCES
        src/Atlas.cpp
        src/PixelClass.cpp
        src/Sprite.cpp
        src/Tileset.cpp
        src/TilesetFile.cpp
        src/TilesetJson.cpp
    DEPENDS
        antwika::enums
        antwika::geometry
        antwika::gfx
        antwika::tilemap
        nlohmann_json::nlohmann_json
)
