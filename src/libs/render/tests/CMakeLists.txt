find_package(GTest REQUIRED)

add_executable(antwika_render_tests
    AtlasSheetsTest.cpp
    CharacterSkinsTest.cpp
    CheckerboardTest.cpp
    HealthBarsTest.cpp
    LightPassesTest.cpp
    ScenePassTest.cpp
    SpritesTest.cpp
    WorldMeshesTest.cpp
    WorldShaderTest.cpp
)

target_link_libraries(antwika_render_tests
    PRIVATE
        antwika::gfx::tests::fakes
        antwika::gfx::tests::mocks
        antwika::render
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_render_tests)
