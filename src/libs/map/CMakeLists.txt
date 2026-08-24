find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME map
    SOURCES
        src/Layers.cpp
        src/MapAssets.cpp
        src/MapFile.cpp
        src/MapFileField.cpp
        src/MapFileMigrations.cpp
        src/MapFileMigrations2.cpp
        src/MapFileMigrations3.cpp
        src/MapFileMigrations4.cpp
        src/MapFileRead2.cpp
        src/MapFileSchema.cpp
        src/MapFileSchema2.cpp
        src/MapFileWrite.cpp
        src/MapFileWrite2.cpp
        src/Marker.cpp
        src/PlayerProgress.cpp
    DEPENDS
        antwika::camera
        antwika::character
        antwika::decor
        antwika::enums
        antwika::gfx
        antwika::image
        antwika::io
        antwika::light
        antwika::schema
        antwika::solver
        antwika::tile
        antwika::tilemap
        antwika::voxel
        antwika::voxelmap
        nlohmann_json::nlohmann_json
)
