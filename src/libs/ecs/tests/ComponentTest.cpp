#include "antwika/ecs/Component.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{

    struct PlainData
    {
        int x{};
        int y{};
    };

    struct HoldsAString
    {
        std::string s;
    };

    static_assert(antwika::ecs::Component<PlainData>);
    static_assert(!antwika::ecs::Component<HoldsAString>);

} // namespace

TEST(ComponentConceptTest, StaticAssertionsAboveProveTheConstraint)
{
    // The interesting assertions are the static_asserts above: a plain
    // struct satisfies Component, one holding a std::string does not.
    // This test exists so the file (and the assertions in it) run as
    // part of the normal test suite rather than living unreferenced.
    SUCCEED();
}
