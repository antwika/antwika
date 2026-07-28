add_library(antwika_reducer_tests_mocks INTERFACE)

add_library(antwika::reducer::tests::mocks
    ALIAS antwika_reducer_tests_mocks)

target_include_directories(antwika_reducer_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_reducer_tests_mocks
    INTERFACE
        antwika::reducer
)

install(
    TARGETS antwika_reducer_tests_mocks
    EXPORT antwika_reducerTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
