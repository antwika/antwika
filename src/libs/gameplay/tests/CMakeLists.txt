find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_gameplay_tests
    ComponentNamesTest.cpp
    GameLoopTest.cpp
    GameTest.cpp
    RosterTest.cpp
)

target_link_libraries(antwika_gameplay_tests
    PRIVATE
        antwika::camera
        antwika::character
        antwika::gameplay
        antwika::gameplay::tests::fakes
        antwika::log::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_gameplay_tests)
