find_package(GTest REQUIRED)

add_executable(antwika_sound_demo_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/DemoTrack.cpp
    DemoLoopTest.cpp
    DemoOptionsTest.cpp
    DemoTrackTest.cpp
)

target_include_directories(antwika_sound_demo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_sound_demo_tests
    PRIVATE
        antwika::cli
        antwika::log::tests::mocks
        antwika::sound
        antwika::time
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_sound_demo_tests)
