#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/holdem/Action.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/holdem/TableView.hpp>
#include <antwika/holdem/fakes/FakeDeck.hpp>

#include "FakeAgent.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/TableSnapshot.hpp"

using antwika::holdem::Action;
using antwika::holdem::Blinds;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::Chips;
using antwika::holdem::fold;
using antwika::holdem::IAgent;
using antwika::holdem::indexOf;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Stage;
using antwika::holdem::StepKind;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::TableView;
using antwika::holdem::fakes::FakeDeck;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::snapshotOf;
using antwika::poker::tests::FakeAgent;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    constexpr std::string_view kCards{
        "Ac Kc Qc Ad Kd Qd 2h 7s 9c Ts Jd"};

    struct Room final
    {
        Table table;
        BankrollLedger ledger;
        CashGame game;
        FakeDeck deck;
        std::vector<FakeAgent> agents;

        explicit Room(std::size_t seats)
            : table(seats, kBlinds),
              game(table, ledger, 100),
              deck(parseCards(kCards)),
              agents(seats)
        {
        }

        void seat(const std::string &player, Chips stack)
        {
            ledger.deposit(player, stack);
            static_cast<void>(game.buyIn(player, stack));
        }

        [[nodiscard]] TableRunner makeRunner()
        {
            std::vector<std::reference_wrapper<IAgent>> refs;
            refs.reserve(agents.size());
            for (auto &agent : agents)
            {
                refs.emplace_back(agent);
            }
            return TableRunner(table, deck, std::move(refs));
        }

        void playHand()
        {
            auto runner = makeRunner();
            for (std::size_t step = 0; step < 64; ++step)
            {
                if (runner.step().kind == StepKind::HandCompleted)
                {
                    return;
                }
            }
        }
    };

    void seatThreeHanded(Room &room)
    {
        room.seat("alice", 300);
        room.seat("bob", 300);
        room.seat("carol", 300);
    }
}

TEST(TableSnapshotTest, SnapshotOf_ReportsAnEmptyTable)
{
    const Room room(3);

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    EXPECT_EQ(snapshot.tableName, "Antwika");
    EXPECT_EQ(snapshot.seats.size(), 3);
    EXPECT_TRUE(snapshot.board.empty());
    EXPECT_EQ(snapshot.pot, 0);
    EXPECT_EQ(snapshot.blinds, kBlinds);
    EXPECT_EQ(snapshot.stage, Stage::PreFlop);
    EXPECT_EQ(snapshot.handsPlayed, 0);
    EXPECT_FALSE(snapshot.handInProgress);

    for (const auto &seat : snapshot.seats)
    {
        EXPECT_TRUE(seat.name.empty());
        EXPECT_EQ(seat.stack, 0);
        EXPECT_FALSE(seat.occupied);
        EXPECT_FALSE(seat.inHand);
        EXPECT_FALSE(seat.isToAct);
    }
}

TEST(TableSnapshotTest, SnapshotOf_NamesWhoeverIsSeated)
{
    Room room(3);
    room.seat("alice", 250);

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    EXPECT_EQ(snapshot.seats.at(0).name, "alice");
    EXPECT_EQ(snapshot.seats.at(0).stack, 250);
    EXPECT_TRUE(snapshot.seats.at(0).occupied);

    EXPECT_TRUE(snapshot.seats.at(1).name.empty());
    EXPECT_FALSE(snapshot.seats.at(1).occupied);
}

TEST(TableSnapshotTest, SnapshotOf_ReportsALiveHand)
{
    Room room(3);
    seatThreeHanded(room);
    room.table.startHand(room.deck);

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    EXPECT_TRUE(snapshot.handInProgress);
    EXPECT_EQ(snapshot.handsPlayed, 1);
    EXPECT_EQ(snapshot.stage, Stage::PreFlop);
    EXPECT_EQ(snapshot.pot, kBlinds.small + kBlinds.big);
    EXPECT_TRUE(snapshot.board.empty());

    for (const auto &seat : snapshot.seats)
    {
        EXPECT_TRUE(seat.inHand);
    }

    const auto buttons = std::ranges::count_if(
        snapshot.seats, [](const auto &seat) { return seat.isButton; });
    const auto toAct = std::ranges::count_if(
        snapshot.seats, [](const auto &seat) { return seat.isToAct; });
    EXPECT_EQ(buttons, 1);
    EXPECT_EQ(toAct, 1);
}

TEST(TableSnapshotTest, SnapshotOf_ShowsEverySeatsHoleCards)
{
    Room room(3);
    seatThreeHanded(room);
    room.table.startHand(room.deck);

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    for (std::size_t index = 0; index < snapshot.seats.size(); ++index)
    {
        EXPECT_EQ(
            snapshot.seats.at(index).holeCards,
            room.table.seatAt(makeSeatId(index)).holeCards);
    }
}

TEST(TableSnapshotTest, SnapshotOf_AgreesWithTheTableOnWhoActs)
{
    Room room(3);
    seatThreeHanded(room);
    room.table.startHand(room.deck);

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");
    const auto expected = room.table.seatToAct();

    ASSERT_TRUE(expected.has_value());
    EXPECT_TRUE(snapshot.seats.at(indexOf(*expected)).isToAct);
    EXPECT_TRUE(
        snapshot.seats.at(indexOf(room.table.button())).isButton);
}

TEST(TableSnapshotTest, SnapshotOf_ReportsTheBoardAsItComesOut)
{
    Room room(3);
    seatThreeHanded(room);
    room.agents[1].script({call()});
    room.agents[2].script({call()});
    room.table.startHand(room.deck);

    auto runner = room.makeRunner();
    for (std::size_t step = 0; step < 64; ++step)
    {
        if (!room.table.board().empty())
        {
            break;
        }
        static_cast<void>(runner.step());
    }

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    ASSERT_FALSE(room.table.board().empty());
    EXPECT_EQ(snapshot.board, room.table.board());
}

TEST(TableSnapshotTest, SnapshotOf_ReportsATableBetweenHands)
{
    Room room(3);
    seatThreeHanded(room);
    room.playHand();

    const auto snapshot = snapshotOf(room.table, room.game, "Antwika");

    EXPECT_FALSE(snapshot.handInProgress);
    EXPECT_EQ(snapshot.handsPlayed, 1);
    EXPECT_EQ(snapshot.pot, 0);

    for (const auto &seat : snapshot.seats)
    {
        EXPECT_FALSE(seat.isToAct);
    }
}
