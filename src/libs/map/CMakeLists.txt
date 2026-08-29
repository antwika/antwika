find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME map
    SOURCES
        src/Layers.cpp
        src/MapFile.cpp
        src/MapFileField.cpp
        src/MapFileMigrationChain.cpp
        src/MapFileMigrationsV12To22.cpp
        src/MapFileMigrationsV1To12.cpp
        src/MapFileMigrationsV22To30.cpp
        src/MapFileMigrationsV30To47.cpp
        src/MapFileMigrationsV47To48.cpp
        src/MapFileMigrationsV48To51.cpp
        src/MapFileRead2.cpp
        src/MapFileSchema.cpp
        src/MapFileSchema2.cpp
        src/MapFileWrite.cpp
        src/MapFileWrite2.cpp
        src/MapWarnings.cpp
        src/PlayerProgress.cpp
    DEPENDS
        antwika::camera
        antwika::decor
        antwika::enums
        antwika::gfx
        antwika::io
        antwika::light
        antwika::loadout
        antwika::schema
        antwika::tile
        antwika::tilemap
        antwika::voxel
        antwika::voxelmap
        nlohmann_json::nlohmann_json
)
