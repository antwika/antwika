add_library(antwika_network_tests_conformance INTERFACE)

add_library(antwika::network::tests::conformance ALIAS
    antwika_network_tests_conformance)

target_include_directories(antwika_network_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_network_tests_conformance
    INTERFACE
        antwika::network
        antwika::log::tests::mocks
)

install(
    TARGETS antwika_network_tests_conformance
    EXPORT antwika_networkTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
