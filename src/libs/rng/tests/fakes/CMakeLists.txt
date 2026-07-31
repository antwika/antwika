add_library(antwika_rng_tests_fakes INTERFACE)

add_library(
    antwika::rng::tests::fakes
    ALIAS antwika_rng_tests_fakes
)

target_include_directories(antwika_rng_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_rng_tests_fakes
    INTERFACE
        antwika::rng
)

install(
    TARGETS antwika_rng_tests_fakes
    EXPORT antwika_rngTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
