find_package(GTest REQUIRED)

add_executable(antwika_console_tests
    ConsoleGatedSinkTest.cpp
    ConsoleSceneTest.cpp
    ConsoleStateTest.cpp
    InputFoldTest.cpp
    JsonSnapshotStoreTest.cpp
    SnapshotCommandsTest.cpp
    SnapshotFormatTest.cpp
    TypingTest.cpp
)

target_link_libraries(antwika_console_tests
    PRIVATE
        antwika::console
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_console_tests)
