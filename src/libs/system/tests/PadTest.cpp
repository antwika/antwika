#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/PadSystem.hpp"

using antwika::component::CheckpointReport;
using antwika::component::ExitReport;
using antwika::component::Pad;
using antwika::component::PadKind;
using antwika::component::Player;
using antwika::component::Position;
using antwika::ecs::Entity;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::GameLoop;
using antwika::gameplay::Phase;
using antwika::log::mocks::MockLogger;
using antwika::system::PadSystem;
using antwika::voxel::VoxelPosition;
using testing::NiceMock;

namespace
{

    /**
     * @brief A walker standing here has its feet in cube (0, 2, 0) and
     * stands on the cube below it.
     */
    constexpr Position kStoodPosition{.x = 1.0F, .y = 4.0F, .z = 1.0F};

    constexpr VoxelPosition kUnderfootCubePosition{.x = 0, .y = 2, .z = 0};

    constexpr VoxelPosition kFootCubePosition{.x = 0, .y = 4, .z = 0};

    constexpr VoxelPosition kFarCubePosition{.x = 20, .y = 4, .z = 20};

    struct PadHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        antwika::system::SimulationState simulationState{};
        PadSystem padSystem{simulationState};

        PadHarness()
        {
            gameLoop.addSystem(Phase::Pickup, padSystem);
        }

        [[nodiscard]] Entity walker(const Position stoodPosition)
        {
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Position>(entity, stoodPosition);
                world.add<Player>(entity, Player{});
            }

            return entity;
        }

        [[nodiscard]] Entity lay(
            const VoxelPosition position, const PadKind kind)
        {
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Pad>(
                    entity,
                    Pad{
                        .position = position,
                        .kind = static_cast<std::uint8_t>(kind)});
            }

            return entity;
        }

        void step(const antwika::time::Tick tick)
        {
            gameLoop.run(tick);
        }
    };

}

TEST(PadTest, Update_ReportsTheCheckpointTheWalkerStandsOn)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kUnderfootCubePosition, PadKind::Checkpoint));
    harness.step(1);

    ASSERT_TRUE(harness.world.has<CheckpointReport>(walker));
    EXPECT_EQ(
        harness.world.get<CheckpointReport>(walker).position,
        kUnderfootCubePosition);
}

TEST(PadTest, Update_ReportsNoCheckpointTheWalkerIsNotOn)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kFarCubePosition, PadKind::Checkpoint));
    harness.step(1);

    EXPECT_FALSE(harness.world.has<CheckpointReport>(walker));
}

TEST(PadTest, Update_LeavesACheckpointUnderTheFeetAloneAsAPadToStandIn)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kFootCubePosition, PadKind::Checkpoint));
    harness.step(1);

    EXPECT_FALSE(harness.world.has<CheckpointReport>(walker));
}

TEST(PadTest, Update_ReportsTheExitTheWalkerStandsIn)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kFootCubePosition, PadKind::Exit));
    harness.step(1);

    ASSERT_TRUE(harness.world.has<ExitReport>(walker));
    EXPECT_EQ(harness.world.get<ExitReport>(walker).position, kFootCubePosition);
}

TEST(PadTest, Update_ReportsTheExitTheWalkerStandsOn)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kUnderfootCubePosition, PadKind::Exit));
    harness.step(1);

    EXPECT_TRUE(harness.world.has<ExitReport>(walker));
}

TEST(PadTest, Update_ReportsNoExitAcrossThePile)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kFarCubePosition, PadKind::Exit));
    harness.step(1);

    EXPECT_FALSE(harness.world.has<ExitReport>(walker));
}

TEST(PadTest, Update_LeavesTheStartPadUnreported)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(harness.lay(kUnderfootCubePosition, PadKind::Start));
    static_cast<void>(harness.lay(kFootCubePosition, PadKind::Start));
    harness.step(1);

    EXPECT_FALSE(harness.world.has<CheckpointReport>(walker));
    EXPECT_FALSE(harness.world.has<ExitReport>(walker));
}

TEST(PadTest, Update_ReadsNoPadWhileTheSimulationIsPaused)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    harness.simulationState.simulationPaused = true;

    static_cast<void>(harness.lay(kUnderfootCubePosition, PadKind::Checkpoint));
    static_cast<void>(harness.lay(kFootCubePosition, PadKind::Exit));
    harness.step(1);

    EXPECT_FALSE(harness.world.has<CheckpointReport>(walker));
    EXPECT_FALSE(harness.world.has<ExitReport>(walker));
}

TEST(PadTest, Update_ReportsThePadToTheWalkerAloneAndNotToThePad)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);
    const auto pad = harness.lay(kUnderfootCubePosition, PadKind::Checkpoint);

    harness.step(1);

    EXPECT_TRUE(harness.world.has<CheckpointReport>(walker));
    EXPECT_FALSE(harness.world.has<CheckpointReport>(pad));
}

TEST(PadTest, Update_ReportsThePadTheWalkerKeepsStandingOnJustTheOnce)
{
    PadHarness harness;
    const auto walker = harness.walker(kStoodPosition);

    static_cast<void>(
        harness.lay(kUnderfootCubePosition, PadKind::Checkpoint));
    static_cast<void>(harness.lay(kFootCubePosition, PadKind::Exit));
    harness.step(1);
    harness.step(2);

    EXPECT_TRUE(harness.world.has<CheckpointReport>(walker));
    EXPECT_TRUE(harness.world.has<ExitReport>(walker));
    EXPECT_EQ(
        harness.world.get<CheckpointReport>(walker).position,
        kUnderfootCubePosition);
}
