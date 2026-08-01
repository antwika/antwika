find_package(GTest REQUIRED)

add_executable(antwika_replay_tests
    EventJsonTest.cpp
    MigrationChainTest.cpp
    ReplayCliTest.cpp
    ReplayDeterminismTest.cpp
    ReplayDocumentTest.cpp
    ReplayJsonTest.cpp
    ReplayOutputTest.cpp
    ReplaySourceTest.cpp
    ReplayWriterReaderTest.cpp
    SchemaVersionTest.cpp
)

target_include_directories(antwika_replay_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_replay_tests
    PRIVATE
        antwika::replay
        antwika::simulation
        antwika::log::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_replay_tests)
