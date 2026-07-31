add_library(antwika_sound_tests_conformance INTERFACE)

add_library(antwika::sound::tests::conformance ALIAS
    antwika_sound_tests_conformance)

target_include_directories(antwika_sound_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_sound_tests_conformance
    INTERFACE
        antwika::sound
        antwika::log::tests::mocks
)

install(
    TARGETS antwika_sound_tests_conformance
    EXPORT antwika_soundTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
