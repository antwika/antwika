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
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/PointF.hpp>
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
using antwika::gfx::PointF;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
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

    class FakeTexture final : public ITexture
    {
    public:
        [[nodiscard]] Size size() const override
        {
            return Size{.width = 1, .height = 1};
        }
    };

    class FakeCountingRenderer final : public IRenderer
    {
    public:
        void clear(Color) override {}
        void drawRect(RectF, Color) override {}
        void drawLine(PointF, PointF, Color) override {}
        void drawText(PointF, std::string_view, std::uint32_t, Color) override
        {
        }
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &) override
        {
            ++uploads;

            return std::make_unique<FakeTexture>();
        }
        void drawTexture(const ITexture &, RectF, RectF, Color) override
        {
            ++blits;
        }
        [[nodiscard]] std::unique_ptr<antwika::gfx::IMesh> createMesh(
            const antwika::gfx::MeshData &) override
        {
            return nullptr;
        }
        void drawMesh(
            const antwika::gfx::IMesh &,
            const antwika::gfx::Mat4 &,
            const antwika::gfx::Camera3D &,
            Color) override
        {
        }
        void pushTransform(const antwika::gfx::Mat4 &) override {}
        void popTransform() override {}
        void present() override { ++presents; }

        std::size_t presents = 0;
        std::size_t uploads = 0;
        std::size_t blits = 0;
    };

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
        FakeCountingRenderer drawnInto;
        std::optional<std::size_t> closeAt;
        bool open = true;
    };

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

    class FakeThrowingBackend final : public IGfxBackend
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
}

TEST(BootstrapTest, Bootstrap_PlaysHandsUntilTheStopEvent)
{
    auto script = threeHandedSession(120);
    ReplaySource source(script);
    std::ostringstream out;

    const auto summary = runRoom(source, out);

    EXPECT_GT(summary.handsPlayed, 5U);
    EXPECT_FALSE(out.str().empty());
}

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

    ASSERT_NE(backend.window, nullptr);
    EXPECT_EQ(backend.window->frames(), kStopAt + 1);
}

TEST(BootstrapTest, Bootstrap_DrawsTheTableAfterItHasStepped)
{
    auto script = threeHandedSession(1);
    ReplaySource source(script);
    std::ostringstream out;
    FakeBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window));

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
    FakeThrowingBackend backend;
    FakeSleeper sleeper;
    const WindowSetup window{.backend = backend, .sleeper = sleeper};

    EXPECT_THROW(
        static_cast<void>(runRoom(source, out, kThreeHandedRoom, &window)),
        GfxError);
}

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
    backend.closeAt = kStopAt + 3;
    const auto windowed =
        runRoom(windowedSource, windowedOut, kThreeHandedRoom, &window);

    EXPECT_EQ(windowed, headless);
    EXPECT_EQ(windowedOut.str(), headlessOut.str());
}

TEST(PrintSummaryTest, PrintSummary_WritesBalancesInNameOrder)
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

TEST(PrintSummaryTest, PrintSummary_MentionsUnwonChips)
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
