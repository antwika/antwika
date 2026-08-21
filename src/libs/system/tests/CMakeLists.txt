find_package(GTest REQUIRED)

add_executable(antwika_system_tests
    HealthTest.cpp
    OrientationTest.cpp
    PatrolTest.cpp
    TickPacerTest.cpp
    WalkerTest.cpp
)

target_link_libraries(antwika_system_tests
    PRIVATE
        antwika::character
        antwika::gameplay
        antwika::log::tests::mocks
        antwika::system
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_system_tests)
