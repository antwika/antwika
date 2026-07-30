#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardScene.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/RenderSystem.hpp"
#include "antwika/life/WindowInputSource.hpp"

#include "ScratchFile.hpp"

using antwika::ecs::ISystem;
using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::CloseRequested;
using antwika::gfx::IWindow;
using antwika::gfx::NullBackend;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::life::Board;
using antwika::life::BoardScene;
using antwika::life::RenderSystem;
using antwika::life::WindowInputSource;
using antwika::life::tests::ScratchFile;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr std::uint32_t kWidth = 8;
    constexpr std::uint32_t kHeight = 8;
    constexpr antwika::time::Tick kMaxTicks = 20;
    constexpr WindowId kWindow{42};

    // Closing the window is what ends this run.
    // So the script deliberately never dispatches engine.stop itself.
    constexpr antwika::time::Tick kTickClosedOn = 4;

    std::vector<TickEvent> gliderSeed()
    {
        std::vector<TickEvent> script;
        for (const auto &payload :
             {R"({"x":1,"y":0})", R"({"x":2,"y":1})", R"({"x":0,"y":2})",
              R"({"x":1,"y":2})", R"({"x":2,"y":2})"})
        {
            script.push_back(
                TickEvent{
                    .tick = 0,
                    .event = Event{
                        .name = antwika::life::events::kToggleCell,
                        .payload = payload,
                    },
                });
        }
        return script;
    }
} // namespace

// The check that says rendering stayed out of the tick path.
// Record a run under a windowed backend, closing the window to end it.
// Replay that recording headlessly and reach the very same board.
// Rendering can only stay outside the tick path if this holds.
TEST(
    RenderDeterminismTest,
    RecordingAWindowedRunReplaysIdenticallyUnderNullBackend)
{
    const ScratchFile replayFile("antwika_life_render_determinism.replay");
    const BoardScene scene;

    // The live run: a mocked window, closed part way through.
    Board liveBoard;
    {
        NiceMock<MockLogger> logger;
        EventRecorder eventSink;
        TickEventRecorder replayRecorder;

        NiceMock<MockRenderer> renderer;
        auto owned = std::make_unique<NiceMock<MockWindow>>();
        ON_CALL(*owned, id()).WillByDefault(Return(kWindow));
        ON_CALL(*owned, renderer()).WillByDefault(ReturnRef(renderer));
        ON_CALL(*owned, size())
            .WillByDefault(Return(Size{.width = 80, .height = 80}));

        NiceMock<MockGfxBackend> backend;
        ON_CALL(backend, createWindow)
            .WillByDefault(
                Return(ByMove(std::unique_ptr<IWindow>(std::move(owned)))));

        // Nothing pending, until the tick the window gets closed on.
        std::size_t polls = 0;
        ON_CALL(backend, pollEvent())
            .WillByDefault(
                [&]() -> std::optional<WindowEvent>
                {
                    ++polls;
                    if (polls == kTickClosedOn + 1)
                    {
                        return WindowEvent{
                            .window = kWindow, .payload = CloseRequested{}};
                    }
                    return std::nullopt;
                });

        const auto opened = backend.createWindow(
            WindowDesc{.title = "test", .size = {.width = 80, .height = 80}});
        RenderSystem renderSystem(*opened, scene, kWidth, kHeight);

        ReplaySource fileSource(gliderSeed());
        WindowInputSource source(fileSource, backend, opened->id());

        std::vector<std::reference_wrapper<ISystem>> observers{renderSystem};
        liveBoard = antwika::life::bootstrap(
            antwika::life::LifeConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .observers = observers,
                .maxTicks = kMaxTicks,
                .replayRecorder = replayRecorder});

        // Through the real save, for the filtering main.cpp relies on.
        // engine.tick must never be fed back in as input.
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(), replayFile.string());
    }

    // The recording must say the run was stopped, and say when.
    const auto recorded = antwika::replay::loadReplayFile(replayFile.string());
    ASSERT_FALSE(recorded.empty());
    EXPECT_EQ(recorded.back().event.name, antwika::engine::events::kStop);
    EXPECT_EQ(recorded.back().tick, kTickClosedOn);

    // The replayed run: the real headless backend, and its real window.
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;
    NullBackend backend(logger);
    const auto window = backend.createWindow(
        WindowDesc{.title = "replay", .size = {.width = 80, .height = 80}});
    RenderSystem renderSystem(*window, scene, kWidth, kHeight);

    ReplaySource fileSource(recorded);
    WindowInputSource source(fileSource, backend, window->id());

    std::vector<std::reference_wrapper<ISystem>> observers{renderSystem};
    const auto replayedBoard = antwika::life::bootstrap(
        antwika::life::LifeConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .width = kWidth,
            .height = kHeight,
            .observers = observers,
            .maxTicks = kMaxTicks});

    EXPECT_EQ(replayedBoard, liveBoard);

    // Two empty boards would agree for the wrong reason.
    EXPECT_NE(
        liveBoard.alive,
        std::vector<bool>(
            static_cast<std::size_t>(kWidth) * kHeight, false));
}
