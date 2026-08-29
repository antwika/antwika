find_package(nlohmann_json REQUIRED)

antwika_add_library(
    NAME loadout
    SOURCES
        src/ComponentValuesJson.cpp
        src/Descriptors.cpp
    DEPENDS
        antwika::component
        nlohmann_json::nlohmann_json
)
