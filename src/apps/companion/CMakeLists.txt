add_executable(antwika_companion
    src/Companion.cpp
    src/FilePetStore.cpp
    src/PacingSink.cpp
    src/Pet.cpp
    src/PetLayout.cpp
    src/PetSave.cpp
    src/PetScene.cpp
    src/PetSink.cpp
    src/PetSnapshot.cpp
    src/RenderSink.cpp
    src/ReviveSink.cpp
    src/TapSink.cpp
    src/main.cpp
)

target_include_directories(antwika_companion
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_companion
    PRIVATE
        antwika::animation
        antwika::app
        antwika::ecs
        antwika::engine
        antwika::gfx
        antwika::gfx_backend
        antwika::i18n
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::replay
        antwika::simulation
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_companion
    ASSETS
        replays/demo.json
)

install(TARGETS antwika_companion
    RUNTIME DESTINATION bin
)
