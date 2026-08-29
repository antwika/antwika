find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(antwika_loadout_tests
    ComponentValuesJsonTest.cpp
    DescriptorsTest.cpp
)

target_link_libraries(antwika_loadout_tests
    PRIVATE
        antwika::loadout
        gtest::gtest
        nlohmann_json::nlohmann_json
)

antwika_bundle_test(TARGET antwika_loadout_tests)
