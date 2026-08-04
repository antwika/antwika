# antwika::testing is build-only: it is test support and is not in this
# module's export set, so an installed consumer must not be told to
# look for it.
antwika_add_test_support(MODULE config KIND conformance
    DEPENDS
        antwika::app
        $<BUILD_INTERFACE:antwika::testing>
)
