find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_schema_tests
    DocumentDepthTest.cpp
    JsonSchemasTest.cpp
    MigrationChainTest.cpp
    PayloadJsonTest.cpp
    SchemaVersionTest.cpp
    StepTest.cpp
    VersionedDocumentTest.cpp
)

target_link_libraries(antwika_schema_tests
    PRIVATE
        antwika::schema
        antwika::schema::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_schema_tests)
