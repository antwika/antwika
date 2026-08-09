find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

antwika_add_library(
    NAME replay
    SOURCES
        src/DocumentDepth.cpp
        src/EventJson.cpp
        src/EventSchema.cpp
        src/JsonShapes.cpp
        src/MigrationChain.cpp
        src/ReplayCli.cpp
        src/ReplayJson.cpp
        src/ReplayMigrations.cpp
        src/ReplayReader.cpp
        src/ReplayRecorder.cpp
        src/ReplaySource.cpp
        src/ReplayWriter.cpp
        src/SchemaVersion.cpp
    DEPENDS
        antwika::cli
        antwika::engine
        antwika::event
        antwika::geometry
        antwika::io
        antwika::log
        antwika::simulation
        antwika::time
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
