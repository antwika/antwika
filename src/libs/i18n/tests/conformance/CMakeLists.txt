add_library(antwika_i18n_tests_conformance INTERFACE)

add_library(antwika::i18n::tests::conformance ALIAS
    antwika_i18n_tests_conformance)

target_include_directories(antwika_i18n_tests_conformance
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_i18n_tests_conformance
    INTERFACE
        antwika::i18n
)

install(
    TARGETS antwika_i18n_tests_conformance
    EXPORT antwika_i18nTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
