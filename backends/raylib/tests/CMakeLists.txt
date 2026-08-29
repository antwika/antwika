find_package(GTest REQUIRED)

if(ANTWIKA_GFX_BACKEND STREQUAL "raylib")
    add_executable(antwika_gfx_backend_raylib_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibBackend.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibMaterial.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibMesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibRenderTarget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibRenderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibRendererDraw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibRendererResources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibResource.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibShader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibTexture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibWindow.cpp
        RaylibBackendConformanceTest.cpp
        RaylibBackendTest.cpp
        RaylibReadPixelsTest.cpp
        RaylibShaderTest.cpp
    )

    target_include_directories(antwika_gfx_backend_raylib_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_gfx_backend_raylib_tests
        PRIVATE
            antwika::gfx
            antwika::gfx::tests::conformance
            antwika::text
            gtest::gtest
            raylib
    )

    antwika_bundle_test(
        TARGET antwika_gfx_backend_raylib_tests
        PROPERTIES RESOURCE_LOCK antwika_gfx_window
    )
endif()

if(ANTWIKA_SOUND_BACKEND STREQUAL "raylib")
    add_executable(antwika_sound_backend_raylib_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibAudioRuntime.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibDevice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibSoundBackend.cpp
        RaylibDeviceTest.cpp
        RaylibSoundBackendConformanceTest.cpp
    )

    target_include_directories(antwika_sound_backend_raylib_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_sound_backend_raylib_tests
        PRIVATE
            antwika::sound
            antwika::sound::tests::conformance
            antwika::log::tests::mocks
            gtest::gtest
            raylib
    )

    antwika_bundle_test(TARGET antwika_sound_backend_raylib_tests)
endif()

if(ANTWIKA_INPUT_BACKEND STREQUAL "raylib")
    add_executable(antwika_input_backend_raylib_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/RaylibInputBackend.cpp
        RaylibInputBackendConformanceTest.cpp
    )

    target_include_directories(antwika_input_backend_raylib_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    target_link_libraries(antwika_input_backend_raylib_tests
        PRIVATE
            antwika::input
            antwika::input::tests::conformance
            gtest::gtest
            raylib
    )

    antwika_bundle_test(TARGET antwika_input_backend_raylib_tests)
endif()
