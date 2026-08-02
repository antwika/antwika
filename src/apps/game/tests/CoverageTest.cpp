#include "antwika/game/Coverage.hpp"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Service.hpp"

using antwika::ecs::World;
using antwika::game::Coverage;
using antwika::game::coverageOf;
using antwika::game::setCoverage;
using antwika::game::kCoverageFull;
using antwika::game::Service;
using antwika::game::serviceIndex;
using antwika::log::mocks::MockLogger;

namespace
{
    class CoverageTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

// The seam every later rule is written against.
// A building nothing has reached is uncovered rather than unknown.
TEST_F(CoverageTest, CoverageOf_AnswersZeroForAnEntityWithNoComponent)
{
    const auto entity = world.create();
    world.commit();

    EXPECT_EQ(coverageOf(world, entity), Coverage{});

    for (const auto service : antwika::game::kServices)
    {
        EXPECT_EQ(coverageOf(world, entity, service), 0);
    }
}

TEST_F(CoverageTest, CoverageOf_AnswersEachServiceOutOfTheComponent)
{
    Coverage held;
    held.ticksLeft[serviceIndex(Service::Water)] = kCoverageFull;
    held.ticksLeft[serviceIndex(Service::Safety)] = 7;

    const auto entity = world.create();
    setCoverage(world, entity, held);
    world.commit();

    EXPECT_EQ(coverageOf(world, entity), held);
    EXPECT_EQ(coverageOf(world, entity, Service::Water), kCoverageFull);
    EXPECT_EQ(coverageOf(world, entity, Service::Health), 0);
    EXPECT_EQ(coverageOf(world, entity, Service::Safety), 7);
    EXPECT_EQ(coverageOf(world, entity, Service::Structure), 0);
}

// A total lookup answers about a handle that names nothing.
// Which is what "it need not be alive" in the contract means.
TEST_F(CoverageTest, CoverageOf_AnswersZeroForAnEntityThatIsNotAlive)
{
    const auto entity = world.create();
    setCoverage(world, entity, Coverage{.ticksLeft = {1, 2, 3, 4}});
    world.commit();

    world.destroy(entity);
    world.commit();

    EXPECT_EQ(coverageOf(world, entity), Coverage{});
}

// Retiring an entity walks every component pool there is.
// Including one it was never given anything out of.
TEST_F(CoverageTest, CoverageOf_SurvivesRetiringAnEntityThatHasNone)
{
    const auto covered = world.create();
    setCoverage(world, covered, Coverage{.ticksLeft = {1, 1, 1, 1}});

    const auto bare = world.create();
    world.commit();

    world.destroy(bare);
    world.commit();

    EXPECT_EQ(
        coverageOf(world, covered), (Coverage{.ticksLeft = {1, 1, 1, 1}}));
    EXPECT_EQ(coverageOf(world, bare), Coverage{});
}

TEST_F(CoverageTest, EqualityComparesEveryService)
{
    const Coverage base{.ticksLeft = {1, 2, 3, 4}};

    EXPECT_EQ(base, base);

    for (std::size_t slot = 0; slot < antwika::game::kServiceCount; ++slot)
    {
        Coverage changed = base;
        changed.ticksLeft[slot] += 1;

        EXPECT_NE(base, changed);
    }
}
