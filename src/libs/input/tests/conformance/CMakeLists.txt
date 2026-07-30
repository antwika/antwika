add_library(antwika_input_tests_conformance INTERFACE)

add_library(antwika::input::tests::conformance ALIAS
    antwika_input_tests_conformance)

target_include_directories(antwika_input_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_input_tests_conformance
    INTERFACE
        antwika::input
        antwika::log::tests::mocks
)

install(
    TARGETS antwika_input_tests_conformance
    EXPORT antwika_inputTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
