# antwika::testing is build-only: it is test support and is not in this
# module's export set, so an installed consumer must not be told to
# look for it.
antwika_add_test_support(MODULE console KIND conformance
    DEPENDS
        antwika::event
        antwika::input
        $<BUILD_INTERFACE:antwika::testing>
)
