add_library(antwika_wfc_tests_mocks INTERFACE)

add_library(antwika::wfc::tests::mocks ALIAS antwika_wfc_tests_mocks)

target_include_directories(antwika_wfc_tests_mocks
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_wfc_tests_mocks
    INTERFACE
        antwika::wfc
)

install(
    TARGETS antwika_wfc_tests_mocks
    EXPORT antwika_wfcTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)
