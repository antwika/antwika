find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME tilemap
    SOURCES
        src/Column.cpp
        src/ExpandedMap.cpp
        src/FlowDirection.cpp
        src/MapFile.cpp
        src/MapJson.cpp
        src/Overlay.cpp
        src/TerrainClass.cpp
        src/TileMap.cpp
    DEPENDS
        antwika::enums
        antwika::geometry
        nlohmann_json::nlohmann_json
)
