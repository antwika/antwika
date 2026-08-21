#include <gtest/gtest.h>

#include <string>

#include "antwika/ecs/Component.hpp"

namespace
{

    struct PlainData final
    {
        int x{};
        int y{};
    };

    struct HoldsAString final
    {
        std::string s;
    };

    static_assert(antwika::ecs::Component<PlainData>);
    static_assert(!antwika::ecs::Component<HoldsAString>);

}

TEST(ComponentConceptTest, Component_IsProvedByTheStaticAssertions)
{
    SUCCEED();
}
