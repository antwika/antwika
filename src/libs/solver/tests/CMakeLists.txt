find_package(GTest REQUIRED)

add_executable(antwika_solver_tests
    ChunkGrowTest.cpp
    VoxelWeaveTest.cpp
)

target_include_directories(antwika_solver_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_solver_tests
    PRIVATE
        antwika::solver
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_solver_tests)
