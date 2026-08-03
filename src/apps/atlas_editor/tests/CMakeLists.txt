find_package(GTest REQUIRED)

add_executable(antwika_atlas_editor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/AtlasEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Canvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/OpeningSheet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CanvasView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/EditorUi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Messages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Palette.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PngAtlasStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SceneSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TileGrid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Tool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/UiOverlay.cpp
    CanvasTest.cpp
    CanvasViewTest.cpp
    DefaultSheetSizeTest.cpp
    EditorOptionsTest.cpp
    EditorSinkTest.cpp
    EditorStateTest.cpp
    EditorUiTest.cpp
    MessagesTest.cpp
    PngAtlasStoreTest.cpp
    RunIntegrationTest.cpp
    SceneTest.cpp
    TileGridTest.cpp
)

# The game's include directory, for DefaultSheetSizeTest alone.
# game::atlasSizeOf() is the contract a blank sheet has to meet.
# It is header-only and compile-time, so nothing is linked for it.
# The application target does not get it, and must not: this editor
# builds without apps/game, and only the test knows the two exist.
target_include_directories(antwika_atlas_editor_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../game/include
)

target_link_libraries(antwika_atlas_editor_tests
    PRIVATE
        antwika::app
        antwika::cli
        antwika::engine
        antwika::event::tests::mocks
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::i18n
        antwika::i18n::tests::conformance
        antwika::input
        antwika::log::tests::mocks
        antwika::replay
        antwika::simulation
        antwika::time::tests::fakes
        antwika::ui
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_atlas_editor_tests)
