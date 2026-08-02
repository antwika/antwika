find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

# A replay is a recording of a run: this library is the file format, the
# schema it is validated against, the migrations that bring an older
# document up to it, the --record/--replay flags that name one, and the
# ReplaySource that feeds one back into a loop.
# Running the loop is antwika::simulation's, which is why this library
# depends on it: ReplaySource is a simulation::ITickEventSource, and a live
# run needs none of what is here.
#
# antwika::cli comes in for ReplayCli, which is the --record/--replay
# table and nothing else: the parsing itself lives there, so a program
# that only wants flags does not link this library to get them.
# antwika::gfx comes in for the gfx::Size a replay records its canvas
# as, and antwika::log for the one thing this library reports rather
# than throws: a recording made against another canvas.
# All of them are one-way: none of those libraries knows this one
# exists.
antwika_add_library(
    NAME replay
    SOURCES
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
        antwika::gfx
        antwika::log
        antwika::simulation
        antwika::time
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
