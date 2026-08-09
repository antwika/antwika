find_package(GTest REQUIRED)

add_subdirectory(conformance)

add_executable(antwika_i18n_tests
    CatalogueTest.cpp
    CompiledMessagesTest.cpp
    LocaleTest.cpp
    LookupTest.cpp
    MessageSetTest.cpp
    MessageTableTest.cpp
    MessagesTest.cpp
    SubstituteTest.cpp
    TranslatorTest.cpp
)

target_link_libraries(antwika_i18n_tests
    PRIVATE
        antwika::i18n
        antwika::i18n::tests::conformance
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_i18n_tests)
