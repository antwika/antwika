find_package(nlohmann_json REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)

# A config file is an application's tuning as a versioned JSON document,
# and this library is the part of reading one that is the same for every
# application: the envelope a document states its magic and version in,
# the schema fragment a bounded whole number is refused by, the
# parse -> read version -> migrate -> validate pipeline, and the stream
# and file plumbing around it.
# What a given application's document *means* -- its magic, its fields,
# its defaults and its decode -- stays in that application, which is why
# nothing here names one.
#
# antwika::replay comes in for MigrationChain and readVersionedDocument,
# which docs/schema-versioning.md makes the one migration mechanism for
# every persisted format; the dependency is one-way, and replay does not
# know this library exists.
# antwika::io is the open/flush/check discipline, stated once over
# there rather than a second time here.
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
