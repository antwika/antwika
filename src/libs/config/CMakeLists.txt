find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

antwika_add_library(
    NAME config
    SOURCES
        src/ConfigDocument.cpp
    DEPENDS
        antwika::io
        antwika::replay
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator
)
