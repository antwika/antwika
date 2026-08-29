#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/app/WindowedSession.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::WindowedSession;
using antwika::app::WindowedSessionSpec;
using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::gfx::CloseRequested;
using antwika::gfx::Size;
using antwika::gfx::WindowSpec;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockWindow;
using antwika::input::InputEvent;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::log::Level;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr Size kCanvasSize{.width = 640, .height = 480};
    constexpr WindowId kWindow{3};

    class Fixture final
    {
    public:
        Fixture()
        {
            ON_CALL(*window, getId()).WillByDefault(Return(kWindow));
            ON_CALL(*window, getConfiguredSize())
                .WillByDefault(Return(kCanvasSize));
            ON_CALL(*window, getSize()).WillByDefault(Return(kCanvasSize));

            ON_CALL(backend, getName()).WillByDefault(Return("null"));
            ON_CALL(backend, pollEvent())
                .WillByDefault(Return(std::nullopt));
            ON_CALL(backend, createWindow(::testing::_))
                .WillByDefault(
                    [this](const WindowSpec &spec)
                    {
                        askedSpec = spec;

                        return std::unique_ptr<antwika::gfx::IWindow>(
                            std::move(ownedWindow));
                    });
        }

        std::ostringstream outputStream;
        ConsoleLogging logging{outputStream, Level::Info};
        NiceMock<MockGfxBackend> backend;
        std::unique_ptr<NiceMock<MockWindow>> ownedWindow =
            std::make_unique<NiceMock<MockWindow>>();
        NiceMock<MockWindow> *window = ownedWindow.get();
        WindowSpec askedSpec;
    };

    WindowedSessionSpec getSessionSpec()
    {
        WindowedSessionSpec spec;
        spec.name = "Antwika Test";
        spec.windowTitle = "Antwika Test Window";
        spec.canvasSize = kCanvasSize;

        return spec;
    }

    std::string recordingOf(const Event &event)
    {
        const auto path =
            antwika::testing::getScratchPath("WindowedSessionTest");

        antwika::replay::saveReplayFile(
            {TickEvent{.tick = 0, .event = event}}, path.string());

        return path.string();
    }
}

TEST(WindowedSessionTest, Describe_AnnouncesBothBackendsByName)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_THAT(
        fixture.outputStream.str(),
        ::testing::HasSubstr(
            "Antwika Test on backend: null, input: fake"));
}

TEST(WindowedSessionTest, Describe_AsksForTheTitleSizeAndResizing)
{
    Fixture fixture;
    FakeInputBackend input;

    auto spec = getSessionSpec();
    spec.resizable = true;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, spec);

    EXPECT_EQ(fixture.askedSpec.title, "Antwika Test Window");
    EXPECT_EQ(fixture.askedSpec.size, kCanvasSize);
    EXPECT_TRUE(fixture.askedSpec.resizable);
    EXPECT_EQ(&session.getWindow(), fixture.window);
}

TEST(WindowedSessionTest, Describe_AsksForAFixedWindowUnlessToldOtherwise)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_FALSE(fixture.askedSpec.resizable);
}

TEST(WindowedSessionTest, Describe_HandsBackTheWindowsCanvas)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_EQ(session.getCanvas(), kCanvasSize);
}

TEST(WindowedSessionTest, Describe_HandsBackThePipelinesCodec)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    const InputEvent edgeEvent{KeyPressed{.key = Key::A}};
    const antwika::input::InputEventCodec twinCodec;

    const auto encodedEvent = session.getCodec().getEncodedEvent(edgeEvent);

    EXPECT_EQ(encodedEvent, twinCodec.getEncodedEvent(edgeEvent));
    EXPECT_EQ(session.getCodec().getDecodedEvent(encodedEvent), edgeEvent);
}

TEST(WindowedSessionTest, Describe_ReadsTheDeviceWithNoReplay)
{
    Fixture fixture;
    const InputEvent edgeEvent{KeyPressed{.key = Key::A}};
    FakeInputBackend input({edgeEvent});

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_EQ(
        session.source().eventsFor(0),
        (std::vector<Event>{session.getCodec().getEncodedEvent(edgeEvent)}));
}

TEST(WindowedSessionTest, Describe_ReadsNoDeviceWithAReplay)
{
    Fixture fixture;
    const Event scriptedEvent{.name = EventName{"test.scripted"}, .payload = "{}"};
    FakeInputBackend input({InputEvent{KeyPressed{.key = Key::A}}});

    auto spec = getSessionSpec();
    spec.replayPath = recordingOf(scriptedEvent);

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, spec);

    EXPECT_EQ(
        session.source().eventsFor(0), (std::vector<Event>{scriptedEvent}));

    std::error_code errorCode;
    std::filesystem::remove(*spec.replayPath, errorCode);
}

TEST(WindowedSessionTest, Describe_SeedsFromTheDemoRecording)
{
    Fixture fixture;
    const Event scriptedEvent{.name = EventName{"test.demo"}, .payload = "{}"};
    FakeInputBackend input;

    auto spec = getSessionSpec();
    spec.demoReplay = recordingOf(scriptedEvent);

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, spec);

    EXPECT_EQ(
        session.source().eventsFor(0), (std::vector<Event>{scriptedEvent}));

    std::error_code errorCode;
    std::filesystem::remove(spec.demoReplay, errorCode);
}

TEST(WindowedSessionTest, Describe_ThrowsOnAnUnreadableRecording)
{
    Fixture fixture;
    FakeInputBackend input;

    auto spec = getSessionSpec();
    spec.replayPath = "no-such-recording.jsonl";

    EXPECT_THROW(
        WindowedSession(
            fixture.logging.logger(), fixture.backend, input, spec),
        antwika::replay::ReplayFormatError);
}

TEST(WindowedSessionTest, Describe_MapsADevicePositionWhenAsked)
{
    Fixture fixture;
    FakeInputBackend input({InputEvent{
        PointerMoved{.position = Position{.x = 200, .y = 100}}}});

    auto spec = getSessionSpec();
    spec.mapsPointerToCanvas = true;

    ON_CALL(*fixture.window, getSize())
        .WillByDefault(Return(Size{.width = 1280, .height = 960}));

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, spec);

    const auto events = session.source().eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        session.getCodec().getDecodedEvent(events.front()),
        (InputEvent{
            PointerMoved{.position = Position{.x = 100, .y = 50}}}));
}

TEST(WindowedSessionTest, Describe_LeavesADevicePositionAlone)
{
    Fixture fixture;
    FakeInputBackend input({InputEvent{
        PointerMoved{.position = Position{.x = 200, .y = 100}}}});

    ON_CALL(*fixture.window, getSize())
        .WillByDefault(Return(Size{.width = 1280, .height = 960}));

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    const auto events = session.source().eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        session.getCodec().getDecodedEvent(events.front()),
        (InputEvent{
            PointerMoved{.position = Position{.x = 200, .y = 100}}}));
}

TEST(WindowedSessionTest, Describe_PassesTheInputPolicyThrough)
{
    Fixture fixture;
    const std::vector<InputEvent> moveEvents{
        InputEvent{PointerMoved{.position = Position{.x = 1, .y = 1}}},
        InputEvent{PointerMoved{.position = Position{.x = 2, .y = 2}}}};

    Fixture otherFixture;
    FakeInputBackend coalescedBackend(moveEvents);
    FakeInputBackend keptBackend(moveEvents);

    auto spec = getSessionSpec();
    spec.input.coalescePointerMotion = true;

    WindowedSession foldingSession(
        fixture.logging.logger(), fixture.backend, coalescedBackend, spec);
    WindowedSession keepingSession(
        otherFixture.logging.logger(),
        otherFixture.backend,
        keptBackend,
        getSessionSpec());

    EXPECT_EQ(foldingSession.source().eventsFor(0).size(), 1U);
    EXPECT_EQ(keepingSession.source().eventsFor(0).size(), 2U);
}

TEST(WindowedSessionTest, Describe_EndsTheRunOnItsOwnWindowClose)
{
    Fixture fixture;
    FakeInputBackend input;

    EXPECT_CALL(fixture.backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = kWindow, .payload = CloseRequested{}}))
        .WillOnce(Return(std::nullopt));

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_EQ(
        session.source().eventsFor(0),
        (std::vector<Event>{
            Event{.name = antwika::engine::events::kStop}}));
}

TEST(WindowedSessionTest, Describe_ReportsANullBackendAsDrawingNothing)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_TRUE(session.isHeadless());
}

TEST(WindowedSessionTest, Describe_ReportsAnyOtherBackendAsDrawing)
{
    Fixture fixture;
    FakeInputBackend input;

    ON_CALL(fixture.backend, getName()).WillByDefault(Return("raylib"));

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, getSessionSpec());

    EXPECT_FALSE(session.isHeadless());
}
