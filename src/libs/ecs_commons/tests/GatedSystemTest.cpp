#include <gtest/gtest.h>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/Tick.hpp>

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::ecs_commons::GatedSystem;
using antwika::log::mocks::MockLogger;

using ::testing::_;

namespace
{
    class GatedSystemTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        ::testing::NiceMock<MockSystem> inner;
        bool allowed = true;
    };
}

TEST_F(GatedSystemTest, Update_RunsTheInnerSystemWhenAllowed)
{
    EXPECT_CALL(inner, update(_, _)).Times(1);

    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);
}

TEST_F(GatedSystemTest, Update_HandsTheInnerSystemTheSameWorldAndTick)
{
    EXPECT_CALL(inner, update(::testing::Ref(world), 41)).Times(1);

    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 41);
}

TEST_F(GatedSystemTest, Update_StagesNothingWhenRefused)
{
    EXPECT_CALL(inner, update(_, _)).Times(0);

    allowed = false;
    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);
}

TEST_F(GatedSystemTest, Update_AsksAgainOnEveryTick)
{
    EXPECT_CALL(inner, update(_, _)).Times(2);

    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);
    allowed = false;
    gate.update(world, 1);
    allowed = true;
    gate.update(world, 2);
}
