add_executable(antwika_music_editor
    src/EditorKeys.cpp
    src/EditorScene.cpp
    src/EditorSink.cpp
    src/EditorState.cpp
    src/MusicEditor.cpp
    src/PasteSource.cpp
    src/Playback.cpp
    src/RenderSink.cpp
    src/Score.cpp
    src/ScoreFiles.cpp
    src/TrackPreset.cpp
    src/VoiceChain.cpp
    src/main.cpp
)

target_include_directories(antwika_music_editor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_music_editor
    PRIVATE
        antwika::app
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::gfx_backend
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::notation
        antwika::pattern
        antwika::replay
        antwika::sequencer
        antwika::simulation
        antwika::sound
        antwika::sound_backend
        antwika::synth
        antwika::time
        antwika::ui
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_music_editor
)

install(TARGETS antwika_music_editor
    RUNTIME DESTINATION bin
)
