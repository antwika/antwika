find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME map
    SOURCES
        src/Layers.cpp
        src/MapAssets.cpp
        src/MapFile.cpp
        src/MapFileMigrations.cpp
        src/MapFileMigrations2.cpp
        src/MapFileMigrations3.cpp
        src/MapFileMigrations4.cpp
        src/MapFileRead2.cpp
        src/MapFileSchema.cpp
        src/MapFileSchema2.cpp
        src/MapFileWrite.cpp
        src/MapFileWrite2.cpp
        src/PlayerProgress.cpp
    DEPENDS
        antwika::camera
        antwika::enums
        antwika::character
        antwika::decor
        antwika::schema
        antwika::gfx
        antwika::io
        antwika::light
        antwika::solver
        antwika::tilemap
        antwika::tile
        antwika::ui
        antwika::voxel
        antwika::voxelmap
        nlohmann_json::nlohmann_json
)
