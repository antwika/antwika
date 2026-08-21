find_package(GTest REQUIRED)

add_executable(antwika_enums_tests
    EnumerationTest.cpp
    FromNameTest.cpp
    NameTableTest.cpp
)

target_include_directories(antwika_enums_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(antwika_enums_tests
    PRIVATE
        antwika::enums
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_enums_tests)
