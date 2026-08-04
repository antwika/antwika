#include <gtest/gtest.h>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/Tick.hpp>

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs_commons::GatedSystem;
using antwika::log::mocks::MockLogger;

namespace
{
    // Counts the ticks it was offered, which is the whole question.
    class CountingSystem final : public ISystem
    {
    public:
        void update(World &, antwika::time::Tick) override
        {
            ++ran;
        }

        std::size_t ran = 0;
    };

    class GatedSystemTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        CountingSystem inner;
        bool allowed = true;
    };
} // namespace

TEST_F(GatedSystemTest, Update_RunsTheInnerSystemWhenAllowed)
{
    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);

    EXPECT_EQ(inner.ran, 1U);
}

// Staging nothing is what holds a world still.
TEST_F(GatedSystemTest, Update_StagesNothingWhenRefused)
{
    allowed = false;
    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);

    EXPECT_EQ(inner.ran, 0U);
}

// Asked once per tick rather than once at construction.
// So a gate that opens mid-run opens for the very next tick.
TEST_F(GatedSystemTest, Update_AsksAgainOnEveryTick)
{
    GatedSystem gate(inner, [this] { return allowed; });

    gate.update(world, 0);
    allowed = false;
    gate.update(world, 1);
    allowed = true;
    gate.update(world, 2);

    EXPECT_EQ(inner.ran, 2U);
}
