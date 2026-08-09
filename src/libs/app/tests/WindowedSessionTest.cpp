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
#include <antwika/gfx/WindowDesc.hpp>
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
using antwika::app::WindowedSessionDesc;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::CloseRequested;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
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
    constexpr Size kCanvas{.width = 640, .height = 480};
    constexpr WindowId kWindow{3};

    class Fixture final
    {
    public:
        Fixture()
        {
            ON_CALL(*window, id()).WillByDefault(Return(kWindow));
            ON_CALL(*window, configuredSize())
                .WillByDefault(Return(kCanvas));
            ON_CALL(*window, size()).WillByDefault(Return(kCanvas));

            ON_CALL(backend, name()).WillByDefault(Return("null"));
            ON_CALL(backend, pollEvent())
                .WillByDefault(Return(std::nullopt));
            ON_CALL(backend, createWindow(::testing::_))
                .WillByDefault(
                    [this](const WindowDesc &desc)
                    {
                        asked = desc;

                        return std::unique_ptr<antwika::gfx::IWindow>(
                            std::move(owned));
                    });
        }

        std::ostringstream out;
        ConsoleLogging logging{out, Level::Info};
        NiceMock<MockGfxBackend> backend;
        std::unique_ptr<NiceMock<MockWindow>> owned =
            std::make_unique<NiceMock<MockWindow>>();
        NiceMock<MockWindow> *window = owned.get();
        WindowDesc asked;
    };

    WindowedSessionDesc describe()
    {
        WindowedSessionDesc desc;
        desc.name = "Antwika Test";
        desc.windowTitle = "Antwika Test Window";
        desc.canvas = kCanvas;

        return desc;
    }

    std::string recordingOf(const Event &event)
    {
        const auto path =
            antwika::testing::scratchPath("WindowedSessionTest");

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
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_THAT(
        fixture.out.str(),
        ::testing::HasSubstr(
            "Antwika Test on backend: null, input: fake"));
}

TEST(WindowedSessionTest, Describe_AsksForTheTitleSizeAndResizing)
{
    Fixture fixture;
    FakeInputBackend input;

    auto desc = describe();
    desc.resizable = true;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, desc);

    EXPECT_EQ(fixture.asked.title, "Antwika Test Window");
    EXPECT_EQ(fixture.asked.size, kCanvas);
    EXPECT_TRUE(fixture.asked.resizable);
    EXPECT_EQ(&session.window(), fixture.window);
}

TEST(WindowedSessionTest, Describe_AsksForAFixedWindowUnlessToldOtherwise)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_FALSE(fixture.asked.resizable);
}

TEST(WindowedSessionTest, Describe_HandsBackTheWindowsCanvas)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_EQ(session.canvas(), kCanvas);
}

TEST(WindowedSessionTest, Describe_HandsBackThePipelinesCodec)
{
    Fixture fixture;
    FakeInputBackend input;

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    const InputEvent edge{KeyPressed{.key = Key::A}};
    const antwika::input::InputEventCodec twin;

    const auto encoded = session.codec().encode(edge);

    EXPECT_EQ(encoded, twin.encode(edge));
    EXPECT_EQ(session.codec().decode(encoded), edge);
}

TEST(WindowedSessionTest, Describe_ReadsTheDeviceWithNoReplay)
{
    Fixture fixture;
    const InputEvent edge{KeyPressed{.key = Key::A}};
    FakeInputBackend input({edge});

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_EQ(
        session.source().eventsFor(0),
        (std::vector<Event>{session.codec().encode(edge)}));
}

TEST(WindowedSessionTest, Describe_ReadsNoDeviceWithAReplay)
{
    Fixture fixture;
    const Event scripted{.name = "test.scripted", .payload = "{}"};
    FakeInputBackend input({InputEvent{KeyPressed{.key = Key::A}}});

    auto desc = describe();
    desc.replayPath = recordingOf(scripted);

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, desc);

    EXPECT_EQ(
        session.source().eventsFor(0), (std::vector<Event>{scripted}));

    std::error_code ignored;
    std::filesystem::remove(*desc.replayPath, ignored);
}

TEST(WindowedSessionTest, Describe_SeedsFromTheDemoRecording)
{
    Fixture fixture;
    const Event scripted{.name = "test.demo", .payload = "{}"};
    FakeInputBackend input;

    auto desc = describe();
    desc.demoReplay = recordingOf(scripted);

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, desc);

    EXPECT_EQ(
        session.source().eventsFor(0), (std::vector<Event>{scripted}));

    std::error_code ignored;
    std::filesystem::remove(desc.demoReplay, ignored);
}

TEST(WindowedSessionTest, Describe_ThrowsOnAnUnreadableRecording)
{
    Fixture fixture;
    FakeInputBackend input;

    auto desc = describe();
    desc.replayPath = "no-such-recording.jsonl";

    EXPECT_THROW(
        WindowedSession(
            fixture.logging.logger(), fixture.backend, input, desc),
        antwika::replay::ReplayFormatError);
}

TEST(WindowedSessionTest, Describe_MapsADevicePositionWhenAsked)
{
    Fixture fixture;
    FakeInputBackend input({InputEvent{
        PointerMoved{.position = Position{.x = 200, .y = 100}}}});

    auto desc = describe();
    desc.mapsPointerToCanvas = true;

    ON_CALL(*fixture.window, size())
        .WillByDefault(Return(Size{.width = 1280, .height = 960}));

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, desc);

    const auto events = session.source().eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        session.codec().decode(events.front()),
        (InputEvent{
            PointerMoved{.position = Position{.x = 100, .y = 50}}}));
}

TEST(WindowedSessionTest, Describe_LeavesADevicePositionAlone)
{
    Fixture fixture;
    FakeInputBackend input({InputEvent{
        PointerMoved{.position = Position{.x = 200, .y = 100}}}});

    ON_CALL(*fixture.window, size())
        .WillByDefault(Return(Size{.width = 1280, .height = 960}));

    WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    const auto events = session.source().eventsFor(0);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(
        session.codec().decode(events.front()),
        (InputEvent{
            PointerMoved{.position = Position{.x = 200, .y = 100}}}));
}

TEST(WindowedSessionTest, Describe_PassesTheInputPolicyThrough)
{
    Fixture fixture;
    const std::vector<InputEvent> moves{
        InputEvent{PointerMoved{.position = Position{.x = 1, .y = 1}}},
        InputEvent{PointerMoved{.position = Position{.x = 2, .y = 2}}}};

    Fixture other;
    FakeInputBackend coalesced(moves);
    FakeInputBackend kept(moves);

    auto desc = describe();
    desc.input.coalescePointerMotion = true;

    WindowedSession folding(
        fixture.logging.logger(), fixture.backend, coalesced, desc);
    WindowedSession keeping(
        other.logging.logger(), other.backend, kept, describe());

    EXPECT_EQ(folding.source().eventsFor(0).size(), 1U);
    EXPECT_EQ(keeping.source().eventsFor(0).size(), 2U);
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
        fixture.logging.logger(), fixture.backend, input, describe());

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
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_TRUE(session.drawsNothing());
}

TEST(WindowedSessionTest, Describe_ReportsAnyOtherBackendAsDrawing)
{
    Fixture fixture;
    FakeInputBackend input;

    ON_CALL(fixture.backend, name()).WillByDefault(Return("sdl3"));

    const WindowedSession session(
        fixture.logging.logger(), fixture.backend, input, describe());

    EXPECT_FALSE(session.drawsNothing());
}
