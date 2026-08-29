find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

antwika_add_library(
    NAME schema
    SOURCES
        src/DocumentDepth.cpp
        src/JsonSchemas.cpp
        src/MigrationChain.cpp
        src/MigrationRow.cpp
        src/SchemaVersion.cpp
        src/Step.cpp
    DEPENDS
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
