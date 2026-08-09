find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_wfc_tests
    AdjacencyConstraintTest.cpp
    AllDifferentConstraintTest.cpp
    CompatibilityTableTest.cpp
    ConstraintRefsTest.cpp
    DomainTest.cpp
    EntropyDeterminismTest.cpp
    EntropyIndexTest.cpp
    OneDimensionalWfcTest.cpp
    SolverBacktrackingTest.cpp
    SolverCompletenessTest.cpp
    SolverConstructionTest.cpp
    SolverDeterminismTest.cpp
    SolverLargeScaleTest.cpp
    SolverPropagationTest.cpp
    SolverStepLimitTest.cpp
    TrailTest.cpp
    WeightedEntropyTest.cpp
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

antwika_bundle_test(TARGET antwika_wfc_tests)
