#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
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
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"
#include "antwika/poker/WindowSetup.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
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
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::poker::RoomConfig;
using antwika::poker::RoomSummary;
using antwika::poker::WindowSetup;
using antwika::time::fakes::FakeClock;
using antwika::time::fakes::FakeSleeper;
using namespace std::chrono_literals;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 400;

    constexpr RoomConfig kRoom{
        .seatCount = 4,
        .blinds = Blinds{.small = 5, .big = 10},
        .minimumBuyIn = 100,
        .seed = 4242,
    };

    struct Session
    {
        RoomSummary summary;
        std::string narration;
        std::vector<TickEvent> recorded;

        bool operator==(const Session &other) const = default;
    };

    // Draws every frame into nothing.
    // Reports a close once it has drawn enough of them.
    class SpectatorBackend final : public IGfxBackend
    {
    public:
        explicit SpectatorBackend(std::size_t closeAfterFrames)
            : closeAt(closeAfterFrames)
        {
        }

        [[nodiscard]] std::string_view name() const override
        {
            return "spectator";
        }

        [[nodiscard]] std::size_t maxWindows() const override { return 1; }

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &) override
        {
            auto owned = std::make_unique<SpectatorWindow>(frames);
            window = owned.get();
            return owned;
        }

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override
        {
            if (window != nullptr && frames >= closeAt)
            {
                window->shut();
            }
            return std::nullopt;
        }

    private:
        class SpectatorWindow final : public IWindow
        {
        public:
            explicit SpectatorWindow(std::size_t &frames)
                : drawnInto(frames)
            {
            }

            [[nodiscard]] WindowId id() const override
            {
                return WindowId{1};
            }
            [[nodiscard]] bool isOpen() const override { return open; }
            [[nodiscard]] std::string title() const override
            {
                return "Antwika";
            }
            [[nodiscard]] Size size() const override
            {
                return Size{.width = 1024, .height = 640};
            }
            [[nodiscard]] IRenderer &renderer() override
            {
                return drawnInto;
            }
            void setTitle(std::string_view) override {}
            void close() override { open = false; }
            void shut() { open = false; }

        private:
            class Counter final : public IRenderer
            {
            public:
                explicit Counter(std::size_t &frames) : frames(frames) {}

                void clear(Color) override {}
                void drawRect(Rect, Color) override {}
                void drawText(
                    Point, std::string_view, std::uint32_t, Color) override
                {
                }
                // The poker table draws no textures, so these are dead.
                [[nodiscard]] std::unique_ptr<ITexture> createTexture(
                    const Bitmap &) override
                {
                    return nullptr;
                }
                void drawTexture(
                    const ITexture &, Rect, Rect, Color) override
                {
                }
                void present() override { ++frames; }

            private:
                std::size_t &frames;
            };

            Counter drawnInto;
            bool open = true;
        };

        std::size_t closeAt;
        std::size_t frames = 0;
        SpectatorWindow *window = nullptr;
    };

    [[nodiscard]] Session runSession(
        IReplaySource &source, const WindowSetup *window = nullptr)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock clock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Warning);
        EventRecorder eventSink;
        TickEventRecorder replayRecorder;
        std::ostringstream out;

        auto summary = antwika::poker::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            out,
            kRoom,
            kMaxTicks,
            &replayRecorder,
            window);

        return Session{
            .summary = std::move(summary),
            .narration = out.str(),
            .recorded = replayRecorder.getEvents(),
        };
    }

    [[nodiscard]] TickEvent at(
        antwika::time::Tick tick, const char *name, std::string payload)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = name, .payload = std::move(payload)},
        };
    }

    [[nodiscard]] std::vector<TickEvent> liveScript()
    {
        std::vector<TickEvent> script;
        for (const auto *player : {"alice", "bob", "carol"})
        {
            script.push_back(at(
                0,
                antwika::poker::events::kDeposit,
                std::string(R"({"player":")") + player
                    + R"(","amount":800})"));
            script.push_back(at(
                0,
                antwika::poker::events::kBuyIn,
                std::string(R"({"player":")") + player
                    + R"(","amount":300})"));
        }
        // Somebody walks in later, and somebody else walks out.
        script.push_back(at(
            2,
            antwika::poker::events::kDeposit,
            R"({"player":"dave","amount":800})"));
        script.push_back(at(
            60,
            antwika::poker::events::kBuyIn,
            R"({"player":"dave","amount":200})"));
        script.push_back(at(200, antwika::engine::events::kStop, ""));
        return script;
    }
} // namespace

// This is the requirement this project exists for, applied to poker.
// Save a replay from a live run, then load it back.
// The second session plays out identically, down to the chip counts.
// Not one card and not one action is stored in that replay.
// Who walked in with what money is the only thing it holds.
// Everything else follows from the seed and the agents' policies.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameSession)
{
    const auto script = liveScript();

    ReplaySource liveSource(script);
    const auto live = runSession(liveSource);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    const auto replayed = runSession(replaySource);

    EXPECT_EQ(replayed, live);
    EXPECT_GT(live.summary.handsPlayed, 5U);
}

// Only the money coming in and out was ever external input.
// Nothing the engine or the agents generate belongs in a replay.
// So filtering those out leaves exactly the script it was driven with.
TEST(ReplayIntegrationTest, RecordedEventsHoldOnlyTheRoomsOwnInput)
{
    const auto script = liveScript();
    ReplaySource source(script);

    auto recorded = runSession(source).recorded;
    std::erase_if(
        recorded,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick
                   || event.event.name == "Running Antwika Poker";
        });

    EXPECT_EQ(recorded, script);
}

// Closing the window is external input like any other.
// It enters through the IReplaySource, so it lands in the recording.
// Replaying that recording headlessly ends at the same point.
// With the same chips, and no window involved at all.
TEST(ReplayIntegrationTest, ReplayReproducesASessionEndedByClosingTheWindow)
{
    // Far fewer frames than the script's own stop event needs.
    // So the close is what ends this session.
    SpectatorBackend backend(30);
    FakeSleeper sleeper;
    const WindowSetup window{
        .backend = backend, .sleeper = sleeper, .framePeriod = 80ms};

    const auto script = liveScript();
    ReplaySource liveSource(script);
    const auto watched = runSession(liveSource, &window);

    // The stop the window injected is in here.
    // Nothing downstream knows it came from a window.
    auto recorded = watched.recorded;
    std::erase_if(
        recorded,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick
                   || event.event.name == "Running Antwika Poker";
        });
    EXPECT_TRUE(
        std::ranges::any_of(
            recorded,
            [](const TickEvent &event)
            {
                return event.event.name == antwika::engine::events::kStop;
            }));

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(recorded, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    const auto replayed = runSession(replaySource);

    EXPECT_EQ(replayed.summary, watched.summary);
    EXPECT_EQ(replayed.narration, watched.narration);
    EXPECT_LT(watched.summary.handsPlayed, 20U);
}
