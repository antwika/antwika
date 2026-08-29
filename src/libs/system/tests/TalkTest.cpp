#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/TalkIntent.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/TalkSystem.hpp"

using antwika::component::DialogueLine;
using antwika::component::Player;
using antwika::component::Position;
using antwika::component::CharacterIndex;
using antwika::component::Speaker;
using antwika::component::TalkIntent;
using antwika::ecs::Entity;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::GameLoop;
using antwika::gameplay::Phase;
using antwika::log::mocks::MockLogger;
using antwika::system::kTalkRadius;
using antwika::system::TalkSystem;
using testing::NiceMock;

namespace
{

    struct TalkHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        antwika::system::SimulationState simulationState{};
        TalkSystem system{simulationState};

        explicit TalkHarness(const std::size_t characterCount = 4)
        {
            simulationState.characterCount = characterCount;
            gameLoop.addSystem(Phase::Walking, system);
        }

        [[nodiscard]] Entity walker()
        {
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Position>(entity, Position{});
                world.add<Player>(entity, Player{});
            }

            return entity;
        }

        [[nodiscard]] Entity figure(
            const float x, const std::uint32_t characterIndex)
        {
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Position>(entity, Position{.x = x});
                world.add<CharacterIndex>(
                    entity, CharacterIndex{.index = characterIndex});
                world.add<Speaker>(entity, Speaker{});
            }

            return entity;
        }

        void wish(const Entity entity)
        {
            const OpenPhase phase(world);

            world.add<TalkIntent>(entity, TalkIntent{});
        }
    };

}

TEST(TalkTest, Update_MovesTheFigureOntoItsNextLine)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    const auto figure = harness.figure(kTalkRadius / 2.0F, 0);

    harness.wish(walker);
    harness.gameLoop.run(0);

    EXPECT_EQ(harness.world.get<Speaker>(figure).nextLineIndex, 1U);
}

TEST(TalkTest, Update_WritesTheLineTheFigureStoodOn)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    static_cast<void>(harness.figure(kTalkRadius / 2.0F, 3));

    harness.wish(walker);
    harness.gameLoop.run(0);

    ASSERT_TRUE(harness.world.has<DialogueLine>(walker));
    EXPECT_EQ(harness.world.get<DialogueLine>(walker).characterIndex, 3U);
    EXPECT_EQ(harness.world.get<DialogueLine>(walker).lineIndex, 0U);
}

TEST(TalkTest, Update_LeavesAFigureOutOfReachStandingSilent)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    const auto figure = harness.figure(kTalkRadius * 2.0F, 0);

    harness.wish(walker);
    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<DialogueLine>(walker));
    EXPECT_EQ(harness.world.get<Speaker>(figure).nextLineIndex, 0U);
}

TEST(TalkTest, Update_LeavesAFigureBeyondTheRosterStandingSilent)
{
    TalkHarness harness{2};
    const auto walker = harness.walker();
    const auto figure = harness.figure(kTalkRadius / 2.0F, 5);

    harness.wish(walker);
    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<DialogueLine>(walker));
    EXPECT_EQ(harness.world.get<Speaker>(figure).nextLineIndex, 0U);
}

TEST(TalkTest, Update_LetsGoOfTheWishItHasAnswered)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    static_cast<void>(harness.figure(kTalkRadius / 2.0F, 0));

    harness.wish(walker);
    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<TalkIntent>(walker));
}

TEST(TalkTest, Update_LeavesAWalkerWithNoWishAlone)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    const auto figure = harness.figure(kTalkRadius / 2.0F, 0);

    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<DialogueLine>(walker));
    EXPECT_EQ(harness.world.get<Speaker>(figure).nextLineIndex, 0U);
}

TEST(TalkTest, Update_MovesTheFigureOnAgainOnASecondWish)
{
    TalkHarness harness;
    const auto walker = harness.walker();
    const auto figure = harness.figure(kTalkRadius / 2.0F, 0);

    harness.wish(walker);
    harness.gameLoop.run(0);
    harness.wish(walker);
    harness.gameLoop.run(1);

    EXPECT_EQ(harness.world.get<Speaker>(figure).nextLineIndex, 2U);
    EXPECT_EQ(harness.world.get<DialogueLine>(walker).lineIndex, 1U);
}
