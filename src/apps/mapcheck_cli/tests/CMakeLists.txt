antwika_add_app_tests(
    APP mapcheck_cli
    TESTS
        CheckMapsTest.cpp
    LIBS
        antwika::app
        antwika::cli
        antwika::log
        antwika::mapcheck
        antwika::testing
        antwika::tilemap
        gtest::gtest
)
