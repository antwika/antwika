find_package(SDL3 REQUIRED)

add_library(antwika_sdl3_runtime
    src/Sdl3Runtime.cpp
)

target_include_directories(antwika_sdl3_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(antwika_sdl3_runtime
    PUBLIC
        antwika::log
        SDL3::SDL3
)

install(
    TARGETS antwika_sdl3_runtime
    EXPORT antwika_sdl3_runtimeTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    EXPORT antwika_sdl3_runtimeTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

add_library(antwika_sdl3_pump
    src/Sdl3Pump.cpp
)

target_include_directories(antwika_sdl3_pump
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(antwika_sdl3_pump
    PUBLIC
        antwika::log
        antwika_sdl3_runtime
        SDL3::SDL3
)

install(
    TARGETS antwika_sdl3_pump
    EXPORT antwika_sdl3_pumpTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    EXPORT antwika_sdl3_pumpTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

if(ANTWIKA_GFX_BACKEND STREQUAL "sdl3")
    add_library(antwika_gfx_backend
        src/Sdl3Backend.cpp
        src/Sdl3Renderer.cpp
        src/Sdl3Texture.cpp
        src/Sdl3Window.cpp
        src/SelectedBackend.cpp
    )

    add_library(antwika::gfx_backend ALIAS antwika_gfx_backend)

    set_target_properties(antwika_gfx_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_gfx_backend
        PUBLIC
            antwika::gfx
        PRIVATE
            antwika_sdl3_pump
            SDL3::SDL3
    )

    install(
        TARGETS antwika_gfx_backend
        EXPORT antwika_gfx_backendTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        EXPORT antwika_gfx_backendTargets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )
endif()

if(ANTWIKA_INPUT_BACKEND STREQUAL "sdl3")
    add_library(antwika_input_backend
        src/Sdl3Clipboard.cpp
        src/Sdl3InputBackend.cpp
        src/SelectedClipboard.cpp
        src/SelectedInputBackend.cpp
    )

    add_library(antwika::input_backend ALIAS antwika_input_backend)

    set_target_properties(antwika_input_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_input_backend
        PUBLIC
            antwika::input
        PRIVATE
            antwika_sdl3_pump
            SDL3::SDL3
    )

    install(
        TARGETS antwika_input_backend
        EXPORT antwika_input_backendTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        EXPORT antwika_input_backendTargets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )
endif()

if(ANTWIKA_SOUND_BACKEND STREQUAL "sdl3")
    add_library(antwika_sound_backend
        src/Sdl3Device.cpp
        src/Sdl3SoundBackend.cpp
        src/SelectedSoundBackend.cpp
    )

    add_library(antwika::sound_backend ALIAS antwika_sound_backend)

    set_target_properties(antwika_sound_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_sound_backend
        PUBLIC
            antwika::sound
        PRIVATE
            antwika_sdl3_runtime
            SDL3::SDL3
    )

    install(
        TARGETS antwika_sound_backend
        EXPORT antwika_sound_backendTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        EXPORT antwika_sound_backendTargets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )
endif()

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()
