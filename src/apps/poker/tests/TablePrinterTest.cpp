#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <sstream>
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
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/holdem/TableView.hpp>

#include <antwika/holdem/fakes/FakeDeck.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "FakeAgent.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/TablePrinter.hpp"

using antwika::holdem::Action;
using antwika::holdem::Blinds;
using antwika::holdem::bet;
using antwika::holdem::call;
using antwika::holdem::check;
using antwika::holdem::Chips;
using antwika::holdem::fold;
using antwika::holdem::IAgent;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::raiseTo;
using antwika::holdem::StepKind;
using antwika::holdem::StepOutcome;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::holdem::TableView;
using antwika::holdem::fakes::FakeDeck;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::TablePrinter;
using antwika::time::fakes::FakeClock;
using antwika::poker::tests::FakeAgent;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    [[nodiscard]] bool contains(
        const std::string &text, const std::string &needle)
    {
        return text.find(needle) != std::string::npos;
    }

    struct Room final
    {
        Table table;
        BankrollLedger ledger;
        CashGame game;
        FakeDeck deck;
        std::vector<FakeAgent> agents;
        std::ostringstream out;
        FakeClock clock{std::chrono::system_clock::time_point{}};
        TablePrinter printer{out, game, table, clock, "Antwika"};

        Room(std::size_t seats, std::string_view cards, Chips minimumBuyIn)
            : table(seats, kBlinds),
              game(table, ledger, minimumBuyIn),
              deck(parseCards(cards)),
              agents(seats)
        {
        }

        void seat(const std::string &player, Chips stack)
        {
            ledger.deposit(player, stack);
            static_cast<void>(game.buyIn(player, stack));
        }

        void script(std::size_t index, std::vector<Action> actions)
        {
            agents[index].script(std::move(actions));
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
                const auto outcome = runner.step();
                printer.printStep(outcome);
                if (outcome.kind == StepKind::HandCompleted)
                {
                    return;
                }
            }
        }
    };

    constexpr std::string_view kThreeHandedCards{
        "Ac Kc Qc Ad Kd Qd 2h 7s 9c Ts Jd"};

    void seatThreeHanded(Room &room)
    {
        room.seat("alice", 300);
        room.seat("bob", 300);
        room.seat("carol", 300);
    }

    void scriptTheLongHand(Room &room)
    {
        room.script(0, {call(), check(), call(), check(), bet(50)});
        room.script(1, {raiseTo(30), bet(40), check(), call()});
        room.script(2, {call(), check(), fold()});
    }
}

TEST(TablePrinterTest, PrintStep_SaysNothingAboutAnIdleTable)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);

    room.printer.printStep(StepOutcome{.kind = StepKind::TableIdle});

    EXPECT_TRUE(room.out.str().empty());
}

TEST(TablePrinterTest, PrintStep_WritesAFoldedRoundUpAsAHandHistory)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);
    room.script(1, {fold()});
    room.script(2, {fold()});

    room.playHand();

    EXPECT_EQ(
        room.out.str(),
        "\nAntwika Hand #1: Hold'em No Limit (5/10)"
        " - 1970/01/01 00:00:00\n"
        "Table 'Antwika' 3-max Seat #2 is the button\n"
        "Seat 1: alice (300 in chips)\n"
        "Seat 2: bob (300 in chips)\n"
        "Seat 3: carol (300 in chips)\n"
        "carol: posts small blind 5\n"
        "alice: posts big blind 10\n"
        "*** HOLE CARDS ***\n"
        "Dealt to alice [Kc Kd]\n"
        "Dealt to bob [Qc Qd]\n"
        "Dealt to carol [Ac Ad]\n"
        "bob: folds\n"
        "carol: folds\n"
        "Uncalled bet (5) returned to alice\n"
        "alice collected 10 from pot\n"
        "*** SUMMARY ***\n"
        "Total pot 10 | Rake 0\n"
        "Seat 1: alice (big blind) collected (10)\n"
        "Seat 2: bob (button) folded before Flop\n"
        "Seat 3: carol (small blind) folded before Flop\n");
}

TEST(TablePrinterTest, PrintStep_NamesEveryActionTheWayAPlayerWould)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);
    scriptTheLongHand(room);

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "bob: raises 20 to 30\n"));
    EXPECT_TRUE(contains(text, "carol: calls 25\n"));
    EXPECT_TRUE(contains(text, "alice: calls 20\n"));
    EXPECT_TRUE(contains(text, "carol: checks\n"));
    EXPECT_TRUE(contains(text, "bob: bets 40\n"));
    EXPECT_TRUE(contains(text, "carol: folds\n"));
}

TEST(TablePrinterTest, PrintStep_MarksEachStreetAsItsCardsComeOut)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);
    scriptTheLongHand(room);

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "*** FLOP *** [2h 7s 9c]\n"));
    EXPECT_TRUE(contains(text, "*** TURN *** [2h 7s 9c] [Ts]\n"));
    EXPECT_TRUE(contains(text, "*** RIVER *** [2h 7s 9c Ts] [Jd]\n"));
}

TEST(TablePrinterTest, PrintStep_ShowsEveryHandThatReachedTheShowdown)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);
    scriptTheLongHand(room);

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "*** SHOW DOWN ***\n"));
    EXPECT_TRUE(
        contains(text, "alice: shows [Kc Kd] (a pair of Kings)\n"));
    EXPECT_TRUE(contains(text, "bob: shows [Qc Qd] (a pair of Queens)\n"));
    EXPECT_TRUE(contains(text, "alice collected 270 from pot\n"));
}

TEST(TablePrinterTest, PrintStep_SumsUpWhatEachSeatWalkedAwayWith)
{
    Room room(3, kThreeHandedCards, 100);
    seatThreeHanded(room);
    scriptTheLongHand(room);

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "Total pot 270 | Rake 0\n"));
    EXPECT_TRUE(contains(text, "Board [2h 7s 9c Ts Jd]\n"));
    EXPECT_TRUE(contains(
        text,
        "Seat 1: alice (big blind) showed [Kc Kd] and won (270)"
        " with a pair of Kings\n"));
    EXPECT_TRUE(contains(
        text,
        "Seat 2: bob (button) showed [Qc Qd] and lost"
        " with a pair of Queens\n"));
    EXPECT_TRUE(contains(
        text, "Seat 3: carol (small blind) folded on the Flop\n"));
}

TEST(TablePrinterTest, PrintStep_ReturnsTheBetNobodyCouldCover)
{
    Room room(3, "2c Ah 3d Ad Kc 9s 4h 7d Js", 100);
    room.seat("alice", 300);
    room.seat("bob", 100);
    room.script(0, {raiseTo(300)});
    room.script(1, {call(), call()});

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "alice: raises 290 to 300 and is all-in\n"));
    EXPECT_TRUE(contains(text, "bob: calls 90 and is all-in\n"));
    EXPECT_TRUE(contains(text, "Uncalled bet (200) returned to alice\n"));
    EXPECT_TRUE(contains(text, "bob collected 200 from pot\n"));
    EXPECT_FALSE(contains(text, "alice collected"));
    EXPECT_TRUE(contains(text, "Total pot 200 | Rake 0\n"));
    EXPECT_TRUE(contains(
        text,
        "Seat 1: alice (big blind) showed [2c 3d] and lost"
        " with high card King\n"));
    EXPECT_TRUE(contains(
        text,
        "Seat 2: bob (button) (small blind) showed [Ah Ad] and won (200)"
        " with a pair of Aces\n"));
}

TEST(TablePrinterTest, PrintStep_WritesUpAHandSettledByTheDealAlone)
{
    Room room(2, "Ac Kc Ad Kd 2c 3d 4h 7s 9c", 5);
    room.seat("alice", 10);
    room.seat("bob", 5);

    room.playHand();

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "Seat 1: alice (10 in chips)\n"));
    EXPECT_TRUE(contains(text, "Seat 2: bob (5 in chips)\n"));
    EXPECT_TRUE(
        contains(text, "bob: posts small blind 5 and is all-in\n"));
    EXPECT_TRUE(
        contains(text, "alice: posts big blind 10 and is all-in\n"));
    EXPECT_TRUE(contains(text, "Dealt to alice [Ac Ad]\n"));
    EXPECT_TRUE(contains(text, "Uncalled bet (5) returned to alice\n"));
    EXPECT_TRUE(contains(text, "alice collected 10 from pot\n"));
}

TEST(TablePrinterTest, PrintStep_ReportsASeatedPlayerWhoIsNotInTheHand)
{
    Room room(2, "Ac Kc Ad Kd", 100);
    room.seat("alice", 300);

    room.printer.printStep(StepOutcome{.kind = StepKind::HandStarted});

    const auto text = room.out.str();
    EXPECT_TRUE(contains(text, "Seat 1: alice (300 in chips)\n"));
    EXPECT_FALSE(contains(text, "Seat 2:"));
    EXPECT_FALSE(contains(text, "posts"));
    EXPECT_FALSE(contains(text, "Dealt to"));
}

TEST(TablePrinterTest, PrintStep_FallsBackToASeatNumberForAnEmptySeat)
{
    Room room(2, "Ac Kc Ad Kd", 100);

    room.printer.printStep(StepOutcome{
        .kind = StepKind::Acted,
        .prompted = true,
        .seat = makeSeatId(1),
        .action = fold(),
    });

    EXPECT_EQ(room.out.str(), "Seat 2: folds\n");
}
