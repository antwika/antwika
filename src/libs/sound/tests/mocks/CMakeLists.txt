add_library(antwika_sound_tests_mocks INTERFACE)

add_library(antwika::sound::tests::mocks ALIAS antwika_sound_tests_mocks)

target_include_directories(antwika_sound_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_sound_tests_mocks
    INTERFACE
        antwika::sound
)

install(
    TARGETS antwika_sound_tests_mocks
    EXPORT antwika_soundTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
