find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_wfc_tests
    DomainTest.cpp
    CompatibilityTableTest.cpp
    AllDifferentConstraintTest.cpp
    AdjacencyConstraintTest.cpp
    TrailTest.cpp
    EntropyIndexTest.cpp
    SolverPropagationTest.cpp
    SolverConstructionTest.cpp
    SolverBacktrackingTest.cpp
    SolverCompletenessTest.cpp
    SolverDeterminismTest.cpp
    WeightedEntropyTest.cpp
    SolverStepLimitTest.cpp
    SolverLargeScaleTest.cpp
    OneDimensionalWfcTest.cpp
)

target_include_directories(antwika_wfc_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_wfc_tests
    PRIVATE
        antwika::wfc
        antwika::wfc::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_wfc_tests)
