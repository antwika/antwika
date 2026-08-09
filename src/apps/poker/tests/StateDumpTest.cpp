#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/holdem/Card.hpp>

#include "antwika/poker/StateDump.hpp"

using antwika::holdem::Card;
using antwika::holdem::makeSeatId;
using antwika::poker::RoomDump;
using antwika::poker::roomDumpFromJson;
using antwika::poker::roomDumpToJson;
using antwika::poker::StateDumpError;

namespace
{
    [[nodiscard]] RoomDump populatedDump()
    {
        RoomDump dump;

        dump.bits = 424242;

        for (std::size_t index = 0;
             index < antwika::holdem::kCardCount;
             ++index)
        {
            dump.deck.cards[index] = static_cast<Card>(
                antwika::holdem::kCardCount - 1 - index);
        }

        dump.deck.dealt = 7;

        antwika::holdem::Seat seat;
        seat.stack = 990;
        seat.committed = 10;
        seat.roundCommitted = 10;
        seat.occupied = true;
        seat.inHand = true;
        seat.holeCards = {
            static_cast<Card>(3), static_cast<Card>(17)};
        dump.table.seats = {seat, seat};

        antwika::holdem::HandResult result;
        result.pot = 20;
        result.stage = antwika::holdem::Stage::Showdown;
        result.board = {static_cast<Card>(4), static_cast<Card>(9)};
        result.payouts = {{.seat = makeSeatId(1), .amount = 20}};
        result.showdown = {
            {.seat = makeSeatId(1),
             .holeCards = {static_cast<Card>(3), static_cast<Card>(17)},
             .value = static_cast<antwika::holdem::HandValue>(77)}};
        dump.table.result = result;

        dump.table.toAct = makeSeatId(0);
        dump.table.pot = 15;
        dump.table.betting = {.currentBet = 10, .lastRaiseSize = 10};
        dump.table.stage = antwika::holdem::Stage::Flop;
        dump.table.board = {static_cast<Card>(30)};
        dump.table.handCount = 3;
        dump.table.button = makeSeatId(1);
        dump.table.handInProgress = true;

        dump.balances = {{"alice", 100}, {"bob", 0}};
        dump.names = {"alice", "bob"};

        dump.printer.notes = {
            {.roundStake = 10,
             .foldedOn = antwika::holdem::Stage::Turn,
             .dealtIn = true,
             .folded = false},
            {.dealtIn = true}};
        dump.printer.smallBlindSeat = makeSeatId(0);
        dump.printer.bigBlindSeat = makeSeatId(1);
        dump.printer.stage = antwika::holdem::Stage::Flop;
        dump.printer.boardShown = 3;

        return dump;
    }
}

TEST(StateDumpTest, RoundTrip_EveryFieldSurvives)
{
    const auto dump = populatedDump();

    EXPECT_EQ(roomDumpFromJson(roomDumpToJson(dump)), dump);
}

TEST(StateDumpTest, RoundTrip_TheAbsentOptionalsStayAbsent)
{
    auto dump = populatedDump();
    dump.table.result.reset();
    dump.table.toAct.reset();
    dump.table.handInProgress = false;
    dump.printer.smallBlindSeat.reset();
    dump.printer.bigBlindSeat.reset();

    const auto encoded = roomDumpToJson(dump);

    EXPECT_FALSE(encoded.at("table").contains("result"));
    EXPECT_FALSE(encoded.at("table").contains("toAct"));
    EXPECT_EQ(roomDumpFromJson(encoded), dump);
}

TEST(StateDumpTest, FromJson_RefusesAStageThisBuildDoesNotKnow)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["table"]["stage"] = "the-river-of-tears";

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesACardPastTheDeck)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["deck"]["cards"][0] = 52;

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesABoardCardPastTheDeck)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["table"]["board"][0] = 52;

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesACursorPastTheDeck)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["deck"]["dealt"] = 53;

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesAWrongHoleCardCount)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["table"]["seats"][0]["hole"].push_back(1);

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesAShowdownWithOneCard)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded["table"]["result"]["showdown"][0]["hole"].erase(1);

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, FromJson_RefusesAMissingMember)
{
    auto encoded = roomDumpToJson(populatedDump());
    encoded.erase("bits");

    EXPECT_THROW((void)roomDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, OperatorEquals_ComparesTheParts)
{
    const auto base = populatedDump();

    auto drawn = populatedDump();
    drawn.bits = 1;
    EXPECT_NE(base, drawn);

    auto reshuffled = populatedDump();
    reshuffled.deck.dealt = 0;
    EXPECT_NE(base, reshuffled);

    auto reseated = populatedDump();
    reseated.table.pot = 0;
    EXPECT_NE(base, reseated);

    auto repaid = populatedDump();
    repaid.balances["alice"] = 1;
    EXPECT_NE(base, repaid);

    auto renamed = populatedDump();
    renamed.names[0] = "carol";
    EXPECT_NE(base, renamed);

    auto narrated = populatedDump();
    narrated.printer.boardShown = 0;
    EXPECT_NE(base, narrated);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryPrinterNoteField)
{
    const antwika::poker::PrinterNote base{
        .roundStake = 10,
        .foldedOn = antwika::holdem::Stage::Turn,
        .dealtIn = true,
        .folded = false};

    auto restaked = base;
    restaked.roundStake += 1;
    EXPECT_NE(base, restaked);

    auto earlier = base;
    earlier.foldedOn = antwika::holdem::Stage::Flop;
    EXPECT_NE(base, earlier);

    auto sittingOut = base;
    sittingOut.dealtIn = false;
    EXPECT_NE(base, sittingOut);

    auto mucked = base;
    mucked.folded = true;
    EXPECT_NE(base, mucked);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryPrinterMemoryField)
{
    const auto base = populatedDump().printer;

    auto retold = base;
    retold.notes[0].folded = true;
    EXPECT_NE(base, retold);

    auto reposted = base;
    reposted.smallBlindSeat.reset();
    EXPECT_NE(base, reposted);

    auto restraddled = base;
    restraddled.bigBlindSeat.reset();
    EXPECT_NE(base, restraddled);

    auto staged = base;
    staged.stage = antwika::holdem::Stage::River;
    EXPECT_NE(base, staged);

    auto redealt = base;
    redealt.boardShown = 0;
    EXPECT_NE(base, redealt);
}
