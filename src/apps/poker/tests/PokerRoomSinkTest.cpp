#include <gtest/gtest.h>

#include <functional>
#include <sstream>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/BankrollError.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/CashGameError.hpp"
#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerEventError.hpp"
#include "antwika/poker/PokerRoomSink.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/TablePrinter.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::holdem::Blinds;
using antwika::holdem::IAgent;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::fakes::FakeDeck;
using antwika::poker::AgentStyle;
using antwika::poker::BankrollError;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::CashGameError;
using antwika::poker::PokerEventError;
using antwika::poker::PokerRoomSink;
using antwika::poker::PolicyAgent;
using antwika::poker::TablePrinter;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] TickEvent named(const char *name, std::string payload)
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = name, .payload = std::move(payload)},
        };
    }

    struct Room
    {
        Table table{2, kBlinds};
        BankrollLedger ledger;
        CashGame game{table, ledger, 100};
        FakeDeck deck{parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c")};
        std::vector<PolicyAgent> agents;
        std::ostringstream out;
        TablePrinter printer{out, game, table};

        Room()
        {
            agents.emplace_back(AgentStyle::Balanced);
            agents.emplace_back(AgentStyle::Balanced);
        }

        [[nodiscard]] TableRunner makeRunner()
        {
            return TableRunner(
                table,
                deck,
                std::vector<std::reference_wrapper<IAgent>>{
                    agents[0], agents[1]});
        }
    };
} // namespace

TEST(PokerRoomSinkTest, Handle_CreditsADepositToTheBankroll)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"alice","amount":500})"));

    EXPECT_EQ(room.ledger.balanceOf("alice"), 500U);
}

TEST(PokerRoomSinkTest, Handle_SeatsAPlayerOnABuyIn)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);
    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"alice","amount":500})"));

    sink.handle(named(
        antwika::poker::events::kBuyIn,
        R"({"player":"alice","amount":200})"));

    EXPECT_EQ(room.ledger.balanceOf("alice"), 300U);
    EXPECT_EQ(room.table.seatAt(makeSeatId(0)).stack, 200U);
}

// The ceiling on a buy-in is the player's own money.
// The sink does not soften that.
TEST(PokerRoomSinkTest, Handle_RefusesABuyInBeyondTheBankroll)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);
    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"alice","amount":150})"));

    EXPECT_THROW(
        sink.handle(named(
            antwika::poker::events::kBuyIn,
            R"({"player":"alice","amount":200})")),
        BankrollError);
}

TEST(PokerRoomSinkTest, Handle_ReturnsChipsToTheBankrollOnACashOut)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);
    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"alice","amount":500})"));
    sink.handle(named(
        antwika::poker::events::kBuyIn,
        R"({"player":"alice","amount":200})"));

    sink.handle(named(
        antwika::poker::events::kCashOut, R"({"player":"alice"})"));

    EXPECT_EQ(room.ledger.balanceOf("alice"), 500U);
    EXPECT_FALSE(room.table.seatAt(makeSeatId(0)).occupied);
}

TEST(PokerRoomSinkTest, Handle_PropagatesACashOutByANonPlayer)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    EXPECT_THROW(
        sink.handle(named(
            antwika::poker::events::kCashOut, R"({"player":"nobody"})")),
        CashGameError);
}

TEST(PokerRoomSinkTest, Handle_AdvancesTheTableByOneStepPerTick)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);
    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"alice","amount":500})"));
    sink.handle(named(
        antwika::poker::events::kDeposit,
        R"({"player":"bob","amount":500})"));
    sink.handle(named(
        antwika::poker::events::kBuyIn,
        R"({"player":"alice","amount":200})"));
    sink.handle(named(
        antwika::poker::events::kBuyIn,
        R"({"player":"bob","amount":200})"));

    EXPECT_EQ(room.table.handsPlayed(), 0U);
    sink.handle(named(antwika::engine::events::kTick, ""));
    EXPECT_EQ(room.table.handsPlayed(), 1U);
    EXPECT_TRUE(room.table.isHandInProgress());

    sink.handle(named(antwika::engine::events::kTick, ""));
    EXPECT_FALSE(room.out.str().empty());
}

TEST(PokerRoomSinkTest, Handle_DoesNothingForAnUnrelatedEvent)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    sink.handle(named("something.else", ""));

    EXPECT_TRUE(room.ledger.balances().empty());
    EXPECT_TRUE(room.out.str().empty());
}

// A busted stack frees its seat once the hand it lost is over.
// The player can buy in again from what is left of their bankroll.
TEST(PokerRoomSinkTest, Handle_SendsABustedPlayerHomeWhenTheHandEnds)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);
    room.ledger.deposit("alice", 1000);
    room.ledger.deposit("bob", 1000);
    static_cast<void>(room.game.buyIn("alice", 100));
    static_cast<void>(room.game.buyIn("bob", 100));

    // Two balanced agents on aces and kings get all-in pre-flop.
    // So one of them is busted by the end of the first hand.
    for (int tick = 0; tick < 12; ++tick)
    {
        sink.handle(named(antwika::engine::events::kTick, ""));
    }

    EXPECT_EQ(room.ledger.balanceOf("alice"), 900U);
    EXPECT_EQ(room.ledger.balanceOf("bob"), 900U);
    EXPECT_FALSE(room.game.seatOf("bob").has_value());
    EXPECT_TRUE(room.game.seatOf("alice").has_value());
}

TEST(PokerRoomSinkTest, Handle_RejectsADepositPayloadThatIsNotJson)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    EXPECT_THROW(
        sink.handle(named(antwika::poker::events::kDeposit, "not json")),
        PokerEventError);
}

TEST(PokerRoomSinkTest, Handle_RejectsADepositMissingItsAmount)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    EXPECT_THROW(
        sink.handle(named(
            antwika::poker::events::kDeposit, R"({"player":"alice"})")),
        PokerEventError);
}

TEST(PokerRoomSinkTest, Handle_RejectsABuyInWithAnEmptyPlayerName)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    EXPECT_THROW(
        sink.handle(named(
            antwika::poker::events::kBuyIn,
            R"({"player":"","amount":200})")),
        PokerEventError);
}

TEST(PokerRoomSinkTest, Handle_RejectsACashOutPayloadWithAnExtraField)
{
    Room room;
    auto runner = room.makeRunner();
    PokerRoomSink sink(runner, room.game, room.ledger, room.printer);

    EXPECT_THROW(
        sink.handle(named(
            antwika::poker::events::kCashOut,
            R"({"player":"alice","amount":5})")),
        PokerEventError);
}
