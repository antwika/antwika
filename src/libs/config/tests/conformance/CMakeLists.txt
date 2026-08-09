antwika_add_test_support(MODULE config KIND conformance
    DEPENDS
        antwika::app
        $<BUILD_INTERFACE:antwika::testing>
)
