find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

# antwika::ecs comes in for TickPacer, which is an ecs::ISystem,
# antwika::gfx for WindowInputSource and for the gfx::Size a replay
# records its canvas as, and antwika::log for the one thing this library
# reports rather than throws: a recording made against another canvas.
# All three are one-way: none of those libraries knows this one exists.
antwika_add_library(
    NAME replay
    SOURCES
        src/CommandLine.cpp
        src/EngineLoop.cpp
        src/EventJson.cpp
        src/EventSchema.cpp
        src/MigrationChain.cpp
        src/ReplayCli.cpp
        src/ReplayJson.cpp
        src/ReplayMigrations.cpp
        src/ReplayOutput.cpp
        src/ReplayReader.cpp
        src/ReplaySource.cpp
        src/ReplayWriter.cpp
        src/SchemaVersion.cpp
        src/TickPacer.cpp
        src/WindowInputSource.cpp
    DEPENDS
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::log
        antwika::time
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
