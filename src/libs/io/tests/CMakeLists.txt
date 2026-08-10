find_package(GTest REQUIRED)

add_executable(antwika_io_tests
    FileListTest.cpp
    FileTest.cpp
)

target_link_libraries(antwika_io_tests
    PRIVATE
        antwika::io
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_io_tests)
