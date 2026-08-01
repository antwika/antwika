find_package(GTest REQUIRED)

add_executable(antwika_gfx3d_demo_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/CubeMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SpinLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SpinScene.cpp
    CubeMeshTest.cpp
    SpinLoopTest.cpp
    SpinSceneTest.cpp
)

target_include_directories(antwika_gfx3d_demo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_gfx3d_demo_tests
    PRIVATE
        antwika::gfx
        antwika::gfx::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_gfx3d_demo_tests)
