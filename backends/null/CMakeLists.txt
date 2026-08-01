if(ANTWIKA_GFX_BACKEND STREQUAL "null")
    add_library(antwika_gfx_backend
        src/SelectedBackend.cpp
    )

    add_library(antwika::gfx_backend ALIAS antwika_gfx_backend)

    set_target_properties(antwika_gfx_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_gfx_backend
        PUBLIC
            antwika::gfx
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

if(ANTWIKA_SOUND_BACKEND STREQUAL "null")
    add_library(antwika_sound_backend
        src/SelectedSoundBackend.cpp
    )

    add_library(antwika::sound_backend ALIAS antwika_sound_backend)

    set_target_properties(antwika_sound_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_sound_backend
        PUBLIC
            antwika::sound
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

if(ANTWIKA_INPUT_BACKEND STREQUAL "null")
    add_library(antwika_input_backend
        src/SelectedInputBackend.cpp
    )

    add_library(antwika::input_backend ALIAS antwika_input_backend)

    set_target_properties(antwika_input_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_input_backend
        PUBLIC
            antwika::input
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

if(ANTWIKA_NETWORK_BACKEND STREQUAL "null")
    add_library(antwika_network_backend
        src/SelectedNetworkBackend.cpp
    )

    add_library(antwika::network_backend ALIAS antwika_network_backend)

    set_target_properties(antwika_network_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(antwika_network_backend
        PUBLIC
            antwika::network
    )

    install(
        TARGETS antwika_network_backend
        EXPORT antwika_network_backendTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        EXPORT antwika_network_backendTargets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )
endif()
