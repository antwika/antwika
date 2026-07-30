find_package(SDL3 REQUIRED)

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
        SDL3::SDL3
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

# SDL ships its own runtime DLL, which is this backend's business rather
# than any app's.
if(MINGW)
    add_custom_command(TARGET antwika_gfx_backend POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

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
