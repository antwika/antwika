find_package(GTest REQUIRED)

if(ANTWIKA_GFX_BACKEND STREQUAL "sdl3")
    add_executable(antwika_gfx_backend_sdl3_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Backend.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Window.cpp
        Sdl3BackendConformanceTest.cpp
        Sdl3RendererTest.cpp
    )

    target_include_directories(antwika_gfx_backend_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_gfx_backend_sdl3_tests
        PRIVATE
            antwika::gfx
            antwika::gfx::tests::conformance
            antwika_sdl3_pump
            gtest::gtest
            SDL3::SDL3
    )

    antwika_bundle_test(TARGET antwika_gfx_backend_sdl3_tests)
endif()

if(ANTWIKA_SOUND_BACKEND STREQUAL "sdl3")
    add_executable(antwika_sound_backend_sdl3_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3Device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3SoundBackend.cpp
        Sdl3SoundBackendConformanceTest.cpp
    )

    target_include_directories(antwika_sound_backend_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_sound_backend_sdl3_tests
        PRIVATE
            antwika::sound
            antwika::sound::tests::conformance
            antwika::log::tests::mocks
            antwika_sdl3_runtime
            gtest::gtest
            SDL3::SDL3
    )

    antwika_bundle_test(TARGET antwika_sound_backend_sdl3_tests)
endif()

if(ANTWIKA_INPUT_BACKEND STREQUAL "sdl3")
    add_executable(antwika_input_backend_sdl3_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/Sdl3InputBackend.cpp
        Sdl3InputBackendConformanceTest.cpp
        Sdl3InputBackendTest.cpp
    )

    target_include_directories(antwika_input_backend_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_input_backend_sdl3_tests
        PRIVATE
            antwika::input
            antwika::input::tests::conformance
            antwika::log::tests::mocks
            antwika_sdl3_pump
            gtest::gtest
            SDL3::SDL3
    )

    antwika_bundle_test(TARGET antwika_input_backend_sdl3_tests)
endif()
