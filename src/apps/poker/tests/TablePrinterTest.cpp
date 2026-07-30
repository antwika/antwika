#include <gtest/gtest.h>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/TablePrinter.hpp"

using antwika::holdem::Blinds;
using antwika::holdem::IAgent;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::StepKind;
using antwika::holdem::StepOutcome;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::fakes::FakeDeck;
using antwika::poker::AgentStyle;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::PolicyAgent;
using antwika::poker::TablePrinter;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] bool contains(
        const std::string &text, const std::string &needle)
    {
        return text.find(needle) != std::string::npos;
    }

    struct Room
    {
        Table table{2, kBlinds};
        BankrollLedger ledger;
        CashGame game{table, ledger, 100};
        // Hands weak enough that two balanced agents check them down.
        // So every street actually gets printed.
        FakeDeck deck{parseCards("Kc Ad 7d 9h 2c 3d 4h 7s 9c")};
        std::vector<PolicyAgent> agents;
        std::ostringstream out;

        Room()
        {
            ledger.deposit("alice", 1000);
            ledger.deposit("bob", 1000);
            static_cast<void>(game.buyIn("alice", 300));
            static_cast<void>(game.buyIn("bob", 300));
            agents.emplace_back(AgentStyle::Balanced);
            agents.emplace_back(AgentStyle::Balanced);
        }

        [[nodiscard]] std::vector<std::reference_wrapper<IAgent>> refs()
        {
            return {agents[0], agents[1]};
        }
    };
} // namespace

TEST(TablePrinterTest, PrintStep_SaysNothingAboutAnIdleTable)
{
    Room room;
    TablePrinter printer(room.out, room.game, room.table);

    printer.printStep(StepOutcome{.kind = StepKind::TableIdle});

    EXPECT_TRUE(room.out.str().empty());
}

TEST(TablePrinterTest, PrintStep_AnnouncesTheDealWithStacksAndCards)
{
    Room room;
    TablePrinter printer(room.out, room.game, room.table);
    TableRunner runner(room.table, room.deck, room.refs());

    printer.printStep(runner.step());

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "hand 1"));
    EXPECT_TRUE(contains(text, "button on bob"));
    EXPECT_TRUE(contains(text, "alice: 290 chips, dealt Kc 7d"));
    EXPECT_TRUE(contains(text, "bob: 295 chips, dealt Ad 9h"));
}

TEST(TablePrinterTest, PrintStep_NamesTheActionAndTheRunningPot)
{
    Room room;
    TablePrinter printer(room.out, room.game, room.table);
    TableRunner runner(room.table, room.deck, room.refs());
    static_cast<void>(runner.step());
    room.out.str("");

    printer.printStep(runner.step());

    EXPECT_TRUE(contains(room.out.str(), "bob"));
    EXPECT_TRUE(contains(room.out.str(), "(pot "));
}

TEST(TablePrinterTest, PrintStep_AnnouncesEachNewStreetWithTheBoard)
{
    Room room;
    TablePrinter printer(room.out, room.game, room.table);
    TableRunner runner(room.table, room.deck, room.refs());
    for (int step = 0; step < 12; ++step)
    {
        printer.printStep(runner.step());
    }

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "-- flop: 2c 3d 4h"));
}

TEST(TablePrinterTest, PrintStep_ReportsTheShowdownAndThePayouts)
{
    Room room;
    TablePrinter printer(room.out, room.game, room.table);
    TableRunner runner(room.table, room.deck, room.refs());
    for (int step = 0; step < 24; ++step)
    {
        const auto outcome = runner.step();
        printer.printStep(outcome);
        if (outcome.kind == StepKind::HandCompleted)
        {
            break;
        }
    }

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "-- showdown, board 2c 3d 4h 7s 9c, pot "));
    EXPECT_TRUE(contains(text, "shows"));
    EXPECT_TRUE(contains(text, "wins "));
}

// Nobody was asked anything here.
// So there is no action to report, only a result.
TEST(TablePrinterTest, PrintStep_ReportsAHandSettledByTheDealAlone)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 1000);
    ledger.deposit("bob", 1000);
    CashGame game(table, ledger, 5);
    static_cast<void>(game.buyIn("alice", 10));
    static_cast<void>(game.buyIn("bob", 5));
    FakeDeck deck(parseCards("Ac Kc Ad Kd 2c 3d 4h 7s 9c"));
    std::vector<PolicyAgent> agents;
    agents.emplace_back(AgentStyle::Balanced);
    agents.emplace_back(AgentStyle::Balanced);
    std::vector<std::reference_wrapper<IAgent>> refs{
        agents[0], agents[1]};
    TableRunner runner(table, deck, std::move(refs));
    std::ostringstream out;
    TablePrinter printer(out, game, table);

    printer.printStep(runner.step());

    const auto text = out.str();
    EXPECT_TRUE(contains(text, "showdown"));
    EXPECT_TRUE(contains(text, "alice wins 15"));
    EXPECT_FALSE(contains(text, "(pot "));
}

TEST(TablePrinterTest, PrintStep_FallsBackToASeatNumberForAnEmptySeat)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    CashGame game(table, ledger, 100);
    std::ostringstream out;
    TablePrinter printer(out, game, table);

    printer.printStep(StepOutcome{
        .kind = StepKind::Acted,
        .prompted = true,
        .seat = makeSeatId(1),
        .action = antwika::holdem::fold(),
    });

    EXPECT_TRUE(contains(out.str(), "seat 1 fold"));
}

// A player can be seated without being in the hand.
// They sat down mid-hand, or busted and are not yet sent home.
// Either way they have a stack but no cards to show.
TEST(TablePrinterTest, PrintStep_ReportsASeatedPlayerWhoIsNotInTheHand)
{
    Table table(2, kBlinds);
    BankrollLedger ledger;
    ledger.deposit("alice", 1000);
    CashGame game(table, ledger, 100);
    static_cast<void>(game.buyIn("alice", 300));
    std::ostringstream out;
    TablePrinter printer(out, game, table);

    printer.printStep(StepOutcome{.kind = StepKind::HandStarted});

    EXPECT_TRUE(contains(out.str(), "alice: 300 chips\n"));
    EXPECT_FALSE(contains(out.str(), "dealt"));
}
