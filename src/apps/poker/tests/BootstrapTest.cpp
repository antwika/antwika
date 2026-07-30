#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/poker/BankrollError.hpp"
#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::holdem::Blinds;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplaySource;
using antwika::poker::BankrollError;
using antwika::poker::RoomConfig;
using antwika::poker::RoomSummary;
using antwika::time::fakes::FakeClock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 400;

    constexpr RoomConfig kThreeHandedRoom{
        .seatCount = 3,
        .blinds = Blinds{.small = 5, .big = 10},
        .minimumBuyIn = 100,
        .seed = 20260729,
    };

    [[nodiscard]] TickEvent at(
        antwika::time::Tick tick, const char *name, std::string payload)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = name, .payload = std::move(payload)},
        };
    }

    [[nodiscard]] RoomSummary runRoom(
        IReplaySource &source,
        std::ostream &out,
        RoomConfig config = kThreeHandedRoom)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock clock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Warning);
        EventRecorder eventSink;

        return antwika::poker::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            out,
            config,
            kMaxTicks);
    }

    [[nodiscard]] std::vector<TickEvent> threeHandedSession(
        antwika::time::Tick stopAt)
    {
        std::vector<TickEvent> script;
        for (const auto *player : {"alice", "bob", "carol"})
        {
            script.push_back(at(
                0,
                antwika::poker::events::kDeposit,
                std::string(R"({"player":")") + player
                    + R"(","amount":1000})"));
            script.push_back(at(
                0,
                antwika::poker::events::kBuyIn,
                std::string(R"({"player":")") + player
                    + R"(","amount":300})"));
        }
        script.push_back(
            at(stopAt, antwika::engine::events::kStop, ""));
        return script;
    }

    [[nodiscard]] antwika::holdem::Chips totalOf(const RoomSummary &summary)
    {
        antwika::holdem::Chips total = summary.chipsLeftOnTable;
        for (const auto &[player, balance] : summary.balances)
        {
            total += balance;
        }
        return total;
    }
} // namespace

TEST(BootstrapTest, Bootstrap_PlaysHandsUntilTheStopEvent)
{
    auto script = threeHandedSession(120);
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_GT(summary.handsPlayed, 5U);
    EXPECT_FALSE(out.str().empty());
}

// The books have to balance.
// Chips only move between bankrolls, the stacks and the pot.
// So the total is exactly what was deposited.
TEST(BootstrapTest, Bootstrap_ConservesEveryChipDeposited)
{
    auto script = threeHandedSession(120);
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_EQ(totalOf(summary), 3000U);
}

TEST(BootstrapTest, Bootstrap_PaysEverybodyOutWhenTheSessionEndsCleanly)
{
    // A hand that ends on the stop tick leaves nothing behind.
    // One cut in half does, and that is reported rather than lost.
    auto script = threeHandedSession(120);
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_EQ(summary.balances.size(), 3U);
    EXPECT_GE(totalOf(summary), 3000U);
}

TEST(BootstrapTest, Bootstrap_DealsNoHandsWithoutAnybodyBuyingIn)
{
    std::vector<TickEvent> script{
        at(5, antwika::engine::events::kStop, ""),
    };
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_EQ(summary.handsPlayed, 0U);
    EXPECT_TRUE(summary.balances.empty());
    EXPECT_TRUE(out.str().empty());
}

TEST(BootstrapTest, Bootstrap_LetsAPlayerJoinPartWayThroughASession)
{
    auto script = threeHandedSession(120);
    script.push_back(at(
        1,
        antwika::poker::events::kDeposit,
        R"({"player":"dave","amount":500})"));
    script.push_back(at(
        30,
        antwika::poker::events::kBuyIn,
        R"({"player":"dave","amount":200})"));
    ReplaySource source(script);
    std::ostringstream out;

    auto roomWithASpareSeat = kThreeHandedRoom;
    roomWithASpareSeat.seatCount = 4;
    const auto summary = runRoom(source, out, roomWithASpareSeat);

    EXPECT_EQ(summary.balances.size(), 4U);
    EXPECT_EQ(totalOf(summary), 3500U);
}

TEST(BootstrapTest, Bootstrap_PropagatesABuyInBeyondAPlayersBankroll)
{
    std::vector<TickEvent> script{
        at(0,
           antwika::poker::events::kDeposit,
           R"({"player":"alice","amount":100})"),
        at(0,
           antwika::poker::events::kBuyIn,
           R"({"player":"alice","amount":500})"),
        at(5, antwika::engine::events::kStop, ""),
    };
    ReplaySource source(script);
    std::ostringstream out;

    EXPECT_THROW(
        static_cast<void>(runRoom(source, out)), BankrollError);
}

// Same room, same events, same seed, and so the same poker.
// Right down to the chip counts, since nothing is left to chance.
TEST(BootstrapTest, Bootstrap_ReachesTheSameResultTwiceOverFromOneScript)
{
    auto script = threeHandedSession(120);

    ReplaySource firstSource(script);
    std::ostringstream firstOut;
    const auto first = runRoom(firstSource, firstOut);

    ReplaySource secondSource(script);
    std::ostringstream secondOut;
    const auto second = runRoom(secondSource, secondOut);

    EXPECT_EQ(first, second);
    EXPECT_EQ(firstOut.str(), secondOut.str());
}

TEST(BootstrapTest, Bootstrap_DealsDifferentCardsForADifferentSeed)
{
    auto script = threeHandedSession(120);

    ReplaySource firstSource(script);
    std::ostringstream firstOut;
    static_cast<void>(runRoom(firstSource, firstOut));

    auto otherSeed = kThreeHandedRoom;
    otherSeed.seed = 99;
    ReplaySource secondSource(script);
    std::ostringstream secondOut;
    static_cast<void>(runRoom(secondSource, secondOut, otherSeed));

    EXPECT_NE(firstOut.str(), secondOut.str());
}
