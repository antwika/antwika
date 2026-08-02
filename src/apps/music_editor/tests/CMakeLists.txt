find_package(GTest REQUIRED)

add_executable(antwika_music_editor_tests
    EditorKeysTest.cpp
    EditorSceneTest.cpp
    EditorSinkTest.cpp
    EditorStateTest.cpp
    PlaybackTest.cpp
    RenderSinkTest.cpp
    RunIntegrationTest.cpp
    ScoreTest.cpp
    TrackPresetTest.cpp
    VoiceChainTest.cpp
)

target_include_directories(antwika_music_editor_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_sources(antwika_music_editor_tests
    PRIVATE
        ../src/EditorKeys.cpp
        ../src/EditorScene.cpp
        ../src/EditorSink.cpp
        ../src/EditorState.cpp
        ../src/Playback.cpp
        ../src/MusicEditor.cpp
        ../src/RenderSink.cpp
        ../src/Score.cpp
        ../src/TrackPreset.cpp
        ../src/VoiceChain.cpp
)

target_link_libraries(antwika_music_editor_tests
    PRIVATE
        antwika::app
        antwika::engine
        antwika::event
        antwika::event::tests::mocks
        antwika::log::tests::mocks
        antwika::replay
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input
        antwika::simulation
        antwika::time::tests::fakes
        antwika::notation
        antwika::pattern
        antwika::sequencer
        antwika::sound
        antwika::synth
        antwika::ui

        gtest::gtest
)

antwika_bundle_test(TARGET antwika_music_editor_tests)
