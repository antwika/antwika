find_package(SDL3 REQUIRED)

# One archive, so however many seams select sdl3 there is one SDL_Init
# and one SDL_Quit around all of them.  Compiling Sdl3Runtime.cpp into
# each backend would give each its own function-local static, and the
# first one to go out of scope would call SDL_Quit under the others.
#
# It is separate from the pump because sound needs SDL up and needs no
# window queue at all: a build selecting sdl3 for sound alone links this
# and not the pump, and so never asks for a display.
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

# One archive again, so a build selecting sdl3 for both graphics and
# input links one copy of the pump, and therefore one pair of queues.
# Compiling Sdl3Pump.cpp into both backends would give each its own
# function-local static, which is the very starvation the pump exists to
# prevent.
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
        src/SelectedBackend.cpp
        src/Sdl3Backend.cpp
        src/Sdl3Renderer.cpp
        src/Sdl3Texture.cpp
        src/Sdl3Window.cpp
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
        src/SelectedInputBackend.cpp
        src/Sdl3InputBackend.cpp
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

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

# SDL ships its own runtime DLL, which is this backend's business rather
# than any app's.
if(MINGW)
    add_custom_command(TARGET antwika_sdl3_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()
