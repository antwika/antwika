add_library(antwika_input_tests_mocks INTERFACE)

add_library(antwika::input::tests::mocks ALIAS antwika_input_tests_mocks)

target_include_directories(antwika_input_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_input_tests_mocks
    INTERFACE
        antwika::input
)

install(
    TARGETS antwika_input_tests_mocks
    EXPORT antwika_inputTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
