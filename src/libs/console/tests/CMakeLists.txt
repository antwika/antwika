find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(fakes)

add_executable(antwika_console_tests
    ConsoleCommandListTest.cpp
    ConsoleGatedSinkTest.cpp
    ConsoleMountTest.cpp
    ConsoleSceneTest.cpp
    ConsoleScriptTest.cpp
    ConsoleSendTest.cpp
    ConsoleStateTest.cpp
    HistoryRowsTest.cpp
    IJsonSnapshotStoreTest.cpp
    InputFoldTest.cpp
    KeyboardLayoutTest.cpp
    SnapshotCommandsTest.cpp
    SnapshotFormatTest.cpp
    TypingTest.cpp
)

target_link_libraries(antwika_console_tests
    PRIVATE
        antwika::console::tests::fakes
        antwika::console
        antwika::testing
        antwika::ui::tests::support
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_console_tests)
