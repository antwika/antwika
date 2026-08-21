find_package(GTest REQUIRED)

add_executable(antwika_camera_tests
    FlyCameraTest.cpp
)

target_link_libraries(antwika_camera_tests
    PRIVATE
        antwika::camera
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_camera_tests)
