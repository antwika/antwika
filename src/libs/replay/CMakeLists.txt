find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

antwika_add_library(
    NAME replay
    SOURCES
        src/EventJson.cpp
        src/EventSchema.cpp
        src/ReplayCli.cpp
        src/ReplayJson.cpp
        src/ReplayMigrations.cpp
        src/ReplayReader.cpp
        src/ReplayRecorder.cpp
        src/ReplaySource.cpp
        src/ReplayWriter.cpp
    DEPENDS
        antwika::cli
        antwika::schema
        antwika::engine
        antwika::event
        antwika::geometry
        antwika::io
        antwika::log
        antwika::time
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
