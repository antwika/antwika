add_library(antwika_input_tests_fakes INTERFACE)

add_library(antwika::input::tests::fakes ALIAS antwika_input_tests_fakes)

target_include_directories(antwika_input_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_input_tests_fakes
    INTERFACE
        antwika::input
)

install(
    TARGETS antwika_input_tests_fakes
    EXPORT antwika_inputTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
