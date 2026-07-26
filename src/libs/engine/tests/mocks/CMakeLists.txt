add_library(antwika_engine_tests_mocks INTERFACE)

add_library(antwika::engine::tests::mocks ALIAS antwika_engine_tests_mocks)

target_include_directories(antwika_engine_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_engine_tests_mocks
    INTERFACE
        antwika::engine
)

install(
    TARGETS antwika_engine_tests_mocks
    EXPORT antwika_engineTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
