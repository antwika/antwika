add_library(antwika_app_tests_fakes INTERFACE)

add_library(antwika::app::tests::fakes ALIAS antwika_app_tests_fakes)

target_include_directories(antwika_app_tests_fakes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_app_tests_fakes
    INTERFACE
        antwika_app
)

install(
    TARGETS antwika_app_tests_fakes
    EXPORT antwika_appTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
