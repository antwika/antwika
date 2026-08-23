#include <gtest/gtest.h>

#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gameplay/fakes/FakeMarkingSystem.hpp"
#include "antwika/gameplay/fakes/FakeRaisingSystem.hpp"
#include "antwika/gameplay/fakes/FakeTellingSystem.hpp"
#include "antwika/gameplay/fakes/UpdateCount.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
namespace fakes = antwika::gameplay::fakes;

using antwika::gameplay::Phase;
using antwika::gameplay::getPhaseName;
using antwika::gameplay::kPhaseCount;
using antwika::gameplay::kAllPhases;
using antwika::gameplay::GameLoop;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(GameLoopTest, Phases_EndWithHealth)
{
    EXPECT_EQ(kAllPhases.back(), Phase::Health);
    EXPECT_EQ(getPhaseName(Phase::Health), "health");
}

TEST(GameLoopTest, PhaseName_GivesEveryPhaseAName)
{
    std::vector<std::string_view> names;

    for (const auto phase : kAllPhases)
    {
        names.push_back(getPhaseName(phase));
    }

    EXPECT_EQ(names.size(), kPhaseCount);

    for (const auto name : names)
    {
        EXPECT_FALSE(name.empty());
    }
}

TEST(GameLoopTest, Run_TakesThePhasesInTheOrderTheyAreCounted)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    std::vector<int> marks;
    fakes::FakeMarkingSystem walkingSystem(marks, 1);
    fakes::FakeMarkingSystem sendingSystem(marks, 0);

    gameLoop.addSystem(Phase::Walking, walkingSystem);
    gameLoop.addSystem(Phase::Sending, sendingSystem);
    gameLoop.run(0);

    EXPECT_EQ(marks, (std::vector<int>{0, 1}));
}

TEST(GameLoopTest, Run_SettlesAPhaseBeforeTheNextBegins)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<fakes::UpdateCount>(entity, fakes::UpdateCount{});
    }

    std::vector<int> seenOrder;
    fakes::FakeRaisingSystem raising(entity);
    fakes::FakeTellingSystem telling(entity, seenOrder);

    gameLoop.addSystem(Phase::Sending, raising);
    gameLoop.addSystem(Phase::Walking, telling);
    gameLoop.run(0);

    EXPECT_EQ(seenOrder, (std::vector<int>{1}));
}

TEST(GameLoopTest, Run_HidesWhatItsOwnPhaseIsStillWriting)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<fakes::UpdateCount>(entity, fakes::UpdateCount{});
    }

    std::vector<int> seenOrder;
    fakes::FakeRaisingSystem raising(entity);
    fakes::FakeTellingSystem telling(entity, seenOrder);

    gameLoop.addSystem(Phase::Sending, raising);
    gameLoop.addSystem(Phase::Sending, telling);
    gameLoop.run(0);

    EXPECT_EQ(seenOrder, (std::vector<int>{0}));
}
