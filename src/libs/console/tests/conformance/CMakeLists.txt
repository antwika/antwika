antwika_add_test_support(MODULE console KIND conformance
    DEPENDS
        antwika::event
        antwika::input
        $<BUILD_INTERFACE:antwika::testing>
)
