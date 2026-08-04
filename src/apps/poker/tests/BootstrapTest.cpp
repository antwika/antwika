#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/poker/BankrollError.hpp"
#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"
#include "antwika/poker/WindowSetup.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::GfxError;
using antwika::gfx::IGfxBackend;
using antwika::gfx::IRenderer;
using antwika::gfx::ITexture;
using antwika::gfx::IWindow;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::holdem::Blinds;
using antwika::log::Level;
using antwika::event::ITickEventSource;
using antwika::replay::ReplaySource;
using antwika::poker::BankrollError;
using antwika::poker::RoomConfig;
using antwika::poker::RoomSummary;
using antwika::poker::WindowSetup;
using antwika::time::fakes::FakeClock;
using antwika::time::fakes::FakeSleeper;
using antwika::log::mocks::MockLogger;
using namespace std::chrono_literals;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 400;

    constexpr WindowId kOurWindow{1};

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
        ITickEventSource &source,
        std::ostream &out,
        RoomConfig config = kThreeHandedRoom,
        const WindowSetup *window = nullptr)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock clock(time);
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;

        antwika::poker::RoomSetup setup{
            .clock = clock,
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .out = out,
            .room = config,
            .maxTicks = kMaxTicks};
        if (window != nullptr)
        {
            setup.window = *window;
        }

        return antwika::poker::bootstrap(setup);
    }

    // An uploaded atlas, opaque to everything that holds it.
    class FakeTexture final : public ITexture
    {
    public:
        [[nodiscard]] Size size() const override
        {
            return Size{.width = 1, .height = 1};
        }
    };

    // Counts frames, and stands in for a renderer nobody inspects.
    class CountingRenderer final : public IRenderer
    {
    public:
        void clear(Color) override {}
        void drawRect(Rect, Color) override {}
        void drawLine(Point, Point, Color) override {}
        void drawText(Point, std::string_view, std::uint32_t, Color) override
        {
        }
        // A texture that is never looked at, only handed back.
        // The scene needs one to have anything to blit through.
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &) override
        {
            ++uploads;

            return std::make_unique<FakeTexture>();
        }
        void drawTexture(const ITexture &, Rect, Rect, Color) override
        {
            ++blits;
        }
        void present() override { ++presents; }

        std::size_t presents = 0;
        std::size_t uploads = 0;
        std::size_t blits = 0;
    };

    // A window that really opens and closes.
    // Three collaborators read isOpen() within one tick.
    // A call-count script would say nothing about what each of them saw.
    class FakeWindow final : public IWindow
    {
    public:
        [[nodiscard]] WindowId id() const override { return kOurWindow; }

        [[nodiscard]] bool isOpen() const override { return open; }

        [[nodiscard]] std::string title() const override { return "Antwika"; }

        [[nodiscard]] Size size() const override
        {
            return Size{.width = 1024, .height = 640};
        }

        [[nodiscard]] bool isFullscreen() const override { return false; }

        [[nodiscard]] IRenderer &renderer() override { return drawnInto; }

        void setTitle(std::string_view) override {}

        void setFullscreen(bool) override {}

        void close() override { open = false; }

        [[nodiscard]] std::size_t frames() const noexcept
        {
            return drawnInto.presents;
        }

        [[nodiscard]] std::size_t uploads() const noexcept
        {
            return drawnInto.uploads;
        }

        [[nodiscard]] std::size_t blits() const noexcept
        {
            return drawnInto.blits;
        }

        // Like somebody closing the window after enough frames.
        void closeAfter(std::optional<std::size_t> frames)
        {
            closeAt = frames;
        }

        void closeIfSeenEnough()
        {
            if (closeAt.has_value() && frames() >= *closeAt)
            {
                open = false;
            }
        }

    private:
        CountingRenderer drawnInto;
        std::optional<std::size_t> closeAt;
        bool open = true;
    };

    // A close request only ever arrives from the backend's queue.
    class FakeBackend final : public IGfxBackend
    {
    public:
        [[nodiscard]] std::string_view name() const override
        {
            return "fake";
        }

        [[nodiscard]] std::size_t maxWindows() const override { return 1; }

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override
        {
            titles.push_back(desc.title);
            auto owned = std::make_unique<FakeWindow>();
            owned->closeAfter(closeAt);
            window = owned.get();
            return owned;
        }

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override
        {
            if (window != nullptr)
            {
                window->closeIfSeenEnough();
            }
            return std::nullopt;
        }

        std::optional<std::size_t> closeAt;
        std::vector<std::string> titles;
        FakeWindow *window = nullptr;
    };

    class ThrowingBackend final : public IGfxBackend
    {
    public:
        [[nodiscard]] std::string_view name() const override
        {
            return "throwing";
        }

        [[nodiscard]] std::size_t maxWindows() const override { return 1; }

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &) override
        {
            throw GfxError("no display");
        }

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override
        {
            return std::nullopt;
        }
    };

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

TEST(BootstrapTest, Bootstrap_OpensNoWindowWhenNotGivenOne)
{
    // Which is the path every other test in this file takes.
    auto script = threeHandedSession(60);
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_GT(summary.handsPlayed, 0U);
}

TEST(BootstrapTest, Bootstrap_NamesTheWindowAfterTheTable)
{
    auto script = threeHandedSession(20);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_EQ(backend.titles.size(), 1);
    EXPECT_EQ(backend.titles.at(0), "Antwika -- Antwika Poker");
}

TEST(BootstrapTest, Bootstrap_DrawsOneFramePerTick)
{
    constexpr antwika::time::Tick kStopAt = 40;
    auto script = threeHandedSession(kStopAt);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    // The loop runs the tick carrying the stop to completion.
    // So the frame count is one past the tick the stop sits on.
    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->frames(), kStopAt + 1);
}

TEST(BootstrapTest, Bootstrap_DrawsTheTableAfterItHasStepped)
{
    // The render sink must sit after the sink that steps the table.
    // Nothing but this test says so.
    // A frame drawn first would show an idle table on the deal.
    auto script = threeHandedSession(1);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    // Tick 0 seats everyone and deals.
    // So the first frame already has a hand in progress.
    EXPECT_NE(out.str().find("Antwika Hand #1"), std::string::npos);
    ASSERT_NE(backend.window, nullptr);
    EXPECT_GT(backend.window->frames(), 0U);
}

TEST(BootstrapTest, Bootstrap_PacesEveryFrameItDraws)
{
    auto script = threeHandedSession(10);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{
        .backend = backend, .sleeper = sleeper, .framePeriod = 80ms};
    backend.closeAt = 4;

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_FALSE(sleeper.requested().empty());
    for (const auto requested : sleeper.requested())
    {
        EXPECT_EQ(requested, 80ms);
    }
}

TEST(BootstrapTest, Bootstrap_StopsWhenTheWindowIsClosed)
{
    // No stop event anywhere in the script.
    // And a tick cap far beyond what the session needs.
    // So returning at all is only possible through the injected stop.
    std::vector<TickEvent> script;
    for (const auto *player : {"alice", "bob", "carol"})
    {
        script.push_back(at(
            0,
            antwika::poker::events::kDeposit,
            std::string(R"({"player":")") + player + R"(","amount":1000})"));
        script.push_back(at(
            0,
            antwika::poker::events::kBuyIn,
            std::string(R"({"player":")") + player + R"(","amount":300})"));
    }
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};
    backend.closeAt = 5;

    const auto summary = runRoom(source, out, kThreeHandedRoom, &window);

    EXPECT_GT(summary.handsPlayed, 0U);
    ASSERT_NE(backend.window, nullptr);
    EXPECT_FALSE(backend.window->isOpen());
}

TEST(BootstrapTest, Bootstrap_HoldsTheFinalFrameUntilTheWindowIsClosed)
{
    constexpr antwika::time::Tick kStopAt = 20;
    constexpr std::size_t kExtraFrames = 3;
    auto script = threeHandedSession(kStopAt);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{
        .backend = backend,
        .sleeper = sleeper,
        .framePeriod = 80ms,
        .holdFinalFrame = true};
    backend.closeAt = kStopAt + 1 + kExtraFrames;

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->frames(), kStopAt + 1 + kExtraFrames);
}

TEST(BootstrapTest, Bootstrap_HoldsNoFinalFrameWhenNobodyAskedToWatch)
{
    // Holding would hang under a backend that never reports a close.
    constexpr antwika::time::Tick kStopAt = 20;
    auto script = threeHandedSession(kStopAt);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->frames(), kStopAt + 1);
    EXPECT_FALSE(backend.window->isOpen());
}

// Pacing and holding the end are two separate answers.
// A paced terminal run is ordinary, and it has to be able to end.
TEST(BootstrapTest, Bootstrap_HoldsNoFinalFrameWhenOnlyPaced)
{
    constexpr antwika::time::Tick kStopAt = 20;
    auto script = threeHandedSession(kStopAt);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{
        .backend = backend, .sleeper = sleeper, .framePeriod = 80ms};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->frames(), kStopAt + 1);
    EXPECT_FALSE(backend.window->isOpen());
}

TEST(BootstrapTest, Bootstrap_PropagatesAGfxErrorFromWindowCreation)
{
    auto script = threeHandedSession(20);
    ReplaySource source(script);
    std::ostringstream out;
    ThrowingBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    EXPECT_THROW(
        static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window)),
        GfxError);
}

// Rendering is a write-only projection of the game.
// So a session that drew itself reaches the same chip counts.
// And tells the same story as one that did not.
TEST(BootstrapTest, Bootstrap_ReachesTheSameResultWithAndWithoutAWindow)
{
    constexpr antwika::time::Tick kStopAt = 120;
    auto script = threeHandedSession(kStopAt);

    ReplaySource headlessSource(script);
    std::ostringstream headlessOut;
    const auto headless = runRoom(headlessSource, headlessOut);

    ReplaySource windowedSource(script);
    std::ostringstream windowedOut;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{
        .backend = backend,
        .sleeper = sleeper,
        .framePeriod = 80ms,
        .holdFinalFrame = true};
    // Watched all the way through, then closed by hand.
    // So this run takes the paced and held-open path.
    backend.closeAt = kStopAt + 3;
    const auto windowed =
        runRoom(windowedSource, windowedOut, kThreeHandedRoom, &window);

    EXPECT_EQ(windowed, headless);
    EXPECT_EQ(windowedOut.str(), headlessOut.str());
}

TEST(PrintSummaryTest, WritesEveryBalanceInNameOrder)
{
    std::ostringstream out;
    const RoomSummary summary{
        .handsPlayed = 3,
        .balances = {{"Ada", 120}, {"Bob", 80}},
        .chipsLeftOnTable = 0,
            .console = {},};

    antwika::poker::printSummary(out, summary);

    EXPECT_EQ(
        out.str(),
        "\n=== 3 hands played ===\n  Ada: 120\n  Bob: 80\n");
}

TEST(PrintSummaryTest, MentionsChipsNobodyHasWonYet)
{
    std::ostringstream out;
    const RoomSummary summary{
        .handsPlayed = 1, .balances = {}, .chipsLeftOnTable = 45,
            .console = {},};

    antwika::poker::printSummary(out, summary);

    EXPECT_EQ(
        out.str(),
        "\n=== 1 hands played ===\n"
        "  (45 chips left in an unfinished hand)\n");
}

// The atlas is uploaded once, by whoever owns the renderer.
// A texture belongs to the renderer that made it.
// So it is uploaded nowhere else, and nothing is blitted without it.
TEST(BootstrapTest, Bootstrap_UploadsTheAtlasAndDrawsThroughIt)
{
    auto script = threeHandedSession(20);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const antwika::gfx::Bitmap atlas{
        .size = {.width = 1, .height = 1},
        .pixels = std::vector<std::uint8_t>{255, 255, 255, 255}};
    const WindowSetup window{
        .backend = backend, .sleeper = sleeper, .atlas = &atlas};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->uploads(), 1U);
    EXPECT_GT(backend.window->blits(), 0U);
}

TEST(BootstrapTest, Bootstrap_UploadsNothingWithoutAnAtlas)
{
    auto script = threeHandedSession(20);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->uploads(), 0U);
    EXPECT_EQ(backend.window->blits(), 0U);
}

// A console needs both its picture and a codec to decode the keys.
// Half a mounting is silently no console rather than half of one.
TEST(BootstrapTest, Bootstrap_AnOverlayWithoutACodecMountsNoConsole)
{
    auto script = threeHandedSession(10);
    ReplaySource source(script);
    std::ostringstream out;
    std::chrono::system_clock::time_point time{};
    FakeClock clock(time);
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    antwika::console::ConsolePicture picture(
        {.width = 1024, .height = 640});

    const auto summary =
        antwika::poker::bootstrap(antwika::poker::RoomSetup{
            .clock = clock,
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .out = out,
            .room = kThreeHandedRoom,
            .consoleOverlay = picture,
            .maxTicks = kMaxTicks});

    EXPECT_TRUE(summary.console.empty());
    EXPECT_TRUE(picture.commands().empty());
}

// The one composition nothing else exercises: a window and a console.
// The render sink is handed the picture, and the opened sheet shows.
TEST(BootstrapTest, Bootstrap_AWindowedRoomPaintsTheConsole)
{
    antwika::input::InputEventCodec codec;
    auto script = threeHandedSession(10);
    script.insert(
        script.end() - 1,
        TickEvent{
            .tick = 1,
            .event = codec.encode(antwika::input::KeyPressed{
                .key = antwika::input::Key::Grave})});
    ReplaySource source(script);
    std::ostringstream out;
    std::chrono::system_clock::time_point time{};
    FakeClock clock(time);
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    antwika::console::ConsolePicture picture(
        {.width = 1024, .height = 640});
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(
        antwika::poker::bootstrap(antwika::poker::RoomSetup{
            .clock = clock,
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .out = out,
            .room = kThreeHandedRoom,
            .codec = codec,
            .consoleOverlay = picture,
            .maxTicks = kMaxTicks,
            .window = window}));

    ASSERT_NE(backend.window, nullptr);
    EXPECT_GT(backend.window->frames(), 0U);
    EXPECT_FALSE(picture.commands().empty());
}
