find_package(raylib REQUIRED)

if(ANTWIKA_GFX_BACKEND STREQUAL "raylib")
    add_library(antwika_gfx_backend
        src/RaylibBackend.cpp
        src/RaylibMaterial.cpp
        src/RaylibMesh.cpp
        src/RaylibRenderer.cpp
        src/RaylibTexture.cpp
        src/RaylibWindow.cpp
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
            raylib
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

if(ANTWIKA_INPUT_BACKEND STREQUAL "raylib")
    add_library(antwika_input_backend
        src/RaylibClipboard.cpp
        src/RaylibInputBackend.cpp
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
            raylib
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
