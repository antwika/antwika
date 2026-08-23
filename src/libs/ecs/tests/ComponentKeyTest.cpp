#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <string_view>

#include <antwika/ecs/ComponentKey.hpp>
#include <antwika/ecs/EcsError.hpp>

using antwika::ecs::ComponentKey;
using antwika::ecs::EcsError;
using antwika::ecs::detail::claimComponentKey;
using antwika::ecs::detail::componentKey;
using antwika::ecs::detail::getKeyOfName;
using antwika::ecs::detail::typeName;

namespace
{

    struct Alpha final
    {
        std::int32_t value = 0;
    };

    struct Beta final
    {
        std::int32_t value = 0;
    };

}

TEST(ComponentKeyTest, TypeName_SpellsTheTypeItIsAskedFor)
{
    EXPECT_NE(typeName<Alpha>().find("Alpha"), std::string_view::npos);
    EXPECT_NE(typeName<Beta>().find("Beta"), std::string_view::npos);
    EXPECT_NE(typeName<Alpha>(), typeName<Beta>());
}

TEST(ComponentKeyTest, ComponentKey_IsSettledBeforeTheProgramRuns)
{
    static_assert(componentKey<Alpha>() == componentKey<Alpha>());
    static_assert(componentKey<Alpha>() != componentKey<Beta>());

    EXPECT_EQ(componentKey<Alpha>(), componentKey<Alpha>());
}

TEST(ComponentKeyTest, ComponentKey_DoesNotDependOnWhichTypeIsTouchedFirst)
{
    const auto beta = componentKey<Beta>();
    const auto alpha = componentKey<Alpha>();

    EXPECT_EQ(alpha, getKeyOfName(typeName<Alpha>()));
    EXPECT_EQ(beta, getKeyOfName(typeName<Beta>()));
}

TEST(ComponentKeyTest, KeyOfName_IsNeverNought)
{
    EXPECT_NE(getKeyOfName(""), 0U);
    EXPECT_NE(getKeyOfName("antwika::component::Position"), 0U);
}

TEST(ComponentKeyTest, KeyOfName_TellsNamesApart)
{
    std::set<ComponentKey> seenKeys;

    for (const auto *name :
         {"a", "b", "antwika::component::Position",
          "antwika::component::Velocity", "antwika::component::Health"})
    {
        EXPECT_TRUE(seenKeys.insert(getKeyOfName(name)).second);
    }
}

TEST(ComponentKeyTest, ClaimComponentKey_LetsOneNameClaimItsKeyTwice)
{
    claimComponentKey(getKeyOfName("one"), "one");

    EXPECT_NO_THROW(claimComponentKey(getKeyOfName("one"), "one"));
}

TEST(ComponentKeyTest, ClaimComponentKey_RefusesTwoNamesOnOneKey)
{
    claimComponentKey(getKeyOfName("first"), "first");

    EXPECT_THROW(
        claimComponentKey(getKeyOfName("first"), "second"), EcsError);
}
