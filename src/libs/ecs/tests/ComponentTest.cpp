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
    // The interesting checks are the static_asserts above this test.
    SUCCEED();
}
