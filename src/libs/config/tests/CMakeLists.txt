find_package(GTest REQUIRED)

add_subdirectory(conformance)

add_executable(antwika_config_tests
    ConfigDocumentTest.cpp
)

target_include_directories(antwika_config_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_config_tests
    PRIVATE
        antwika::config
        antwika::replay
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_config_tests)
