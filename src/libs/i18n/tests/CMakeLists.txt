find_package(GTest REQUIRED)

add_executable(antwika_i18n_tests
    CatalogueTest.cpp
    LocaleTest.cpp
    LookupTest.cpp
    MessageIdTest.cpp
    SubstituteTest.cpp
    TranslatorTest.cpp
)

target_link_libraries(antwika_i18n_tests
    PRIVATE
        antwika::i18n
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_i18n_tests)
