add_library(antwika_holdem_tests_mocks INTERFACE)

add_library(
    antwika::holdem::tests::mocks
    ALIAS antwika_holdem_tests_mocks
)

target_include_directories(antwika_holdem_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_holdem_tests_mocks
    INTERFACE
        antwika::holdem
)

install(
    TARGETS antwika_holdem_tests_mocks
    EXPORT antwika_holdemTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
