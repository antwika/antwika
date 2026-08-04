add_library(antwika_config_tests_conformance INTERFACE)

add_library(antwika::config::tests::conformance ALIAS
    antwika_config_tests_conformance)

target_include_directories(antwika_config_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_config_tests_conformance
    INTERFACE
        antwika::app
        antwika::config
)

install(
    TARGETS antwika_config_tests_conformance
    EXPORT antwika_configTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
