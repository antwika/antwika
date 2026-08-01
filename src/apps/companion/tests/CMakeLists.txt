find_package(GTest REQUIRED)

add_executable(antwika_companion_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Companion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DayMood.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/FilePetStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LifeStage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Lineage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PacingSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Pet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PetLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PetSave.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PetScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PetSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PetSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PropSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ReviveSink.cpp
    CompanionTest.cpp
    DayMoodTest.cpp
    FilePetStoreTest.cpp
    LifeStageTest.cpp
    LineageTest.cpp
    PetLayoutTest.cpp
    PetSaveTest.cpp
    PetSceneTest.cpp
    PetSnapshotTest.cpp
    PetTest.cpp
    ReviveSinkTest.cpp
    RunIntegrationTest.cpp
    SessionPersistenceTest.cpp
    SinksTest.cpp
)

target_include_directories(antwika_companion_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_companion_tests
    PRIVATE
        antwika::animation
        antwika::ecs
        antwika::engine
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::i18n
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_companion_tests)
