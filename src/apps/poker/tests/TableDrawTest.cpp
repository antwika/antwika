#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <utility>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/holdem/CardText.hpp>

#include "antwika/poker/TableScene.hpp"
#include "antwika/poker/TableSnapshot.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::holdem::parseCards;
    using antwika::poker::SeatSnapshot;
    using antwika::poker::Stage;
    using antwika::poker::TableScene;
    using antwika::poker::TableSnapshot;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] SeatSnapshot player(
        std::string name, antwika::poker::Chips stack)
    {
        return SeatSnapshot{
            .name = std::move(name), .stack = stack, .occupied = true};
    }

    [[nodiscard]] TableSnapshot liveTable()
    {
        auto alice = player("alice", 380);
        alice.inHand = true;
        alice.isButton = true;
        alice.holeCards = {parseCards("As Kh")[0], parseCards("As Kh")[1]};

        auto bob = player("bob", 120);
        bob.inHand = true;
        bob.isToAct = true;
        bob.roundCommitted = 40;
        bob.holeCards = {parseCards("7d 7c")[0], parseCards("7d 7c")[1]};

        return TableSnapshot{
            .tableName = "Antwika",
            .seats = {alice, bob},
            .board = parseCards("Ah Kd 7s"),
            .pot = 240,
            .blinds = {.small = 5, .big = 10},
            .stage = Stage::Flop,
            .handsPlayed = 12,
            .handInProgress = true};
    }
}

TEST(TableDrawTest, Draw_DrawsATableOnTheFlop)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const TableScene scene;

    scene.draw(renderer, kCanvas, liveTable());
}
