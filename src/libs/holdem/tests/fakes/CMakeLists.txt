add_library(antwika_holdem_tests_fakes INTERFACE)

add_library(
    antwika::holdem::tests::fakes
    ALIAS antwika_holdem_tests_fakes
)

target_include_directories(antwika_holdem_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_holdem_tests_fakes
    INTERFACE
        antwika::holdem
)

install(
    TARGETS antwika_holdem_tests_fakes
    EXPORT antwika_holdemTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
