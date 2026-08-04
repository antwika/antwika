#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/ui/Pointer.hpp>

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/Action.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BindingSource.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/OptionsFile.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::game::tests::kTranslator;
using antwika::game::tests::widgetCentre;

using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::event::mocks::MockEventSink;
using antwika::game::Action;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::BindingSource;
using antwika::game::BindOutcome;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::kDefaultBindings;
using antwika::game::KeyBindings;
using antwika::game::kUiCanvas;
using antwika::game::machineOptionsFor;
using antwika::game::MainMenuScene;
using antwika::game::OptionsScene;
using antwika::game::OptionsState;
using antwika::game::PathIndex;
using antwika::game::saveOptionsFile;
using antwika::game::UiOverlay;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::input::InputPipelineOptions;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 24;

    // What one machine's owner has bound, and what another's has.
    // Nothing the two agree on.
    // So a run reading the wrong one cannot reach the right city.
    [[nodiscard]] KeyBindings machineOne()
    {
        KeyBindings bindings;
        EXPECT_EQ(bindings.bind(Action::Pause, Key::J), BindOutcome::Bound);
        EXPECT_EQ(bindings.bind(Action::ZoomIn, Key::K), BindOutcome::Bound);
        return bindings;
    }

    [[nodiscard]] KeyBindings machineTwo()
    {
        KeyBindings bindings;
        EXPECT_EQ(bindings.bind(Action::Pause, Key::L), BindOutcome::Bound);
        EXPECT_EQ(bindings.bind(Action::ZoomIn, Key::M), BindOutcome::Bound);
        return bindings;
    }

    struct Played
    {
        GameSummary summary;
        std::vector<TickEvent> recorded;
    };

    [[nodiscard]] Position atCell(Cell cell)
    {
        const auto centre = cellCentre(cell, Camera());
        return Position{.x = centre.x, .y = centre.y};
    }

    [[nodiscard]] Position pixelOnMenu(antwika::ui::WidgetId id)
    {
        const MainMenuScene scene{kTranslator};
        const auto centre = widgetCentre(
            scene.describe(kUiCanvas, antwika::ui::Pointer{}), id);

        return Position{
            .x = centre.value_or(antwika::gfx::Point{}).x,
            .y = centre.value_or(antwika::gfx::Point{}).y};
    }

    [[nodiscard]] Position pixelOnOptions(antwika::ui::WidgetId id)
    {
        const OptionsScene scene{kTranslator};
        const OptionsState state;
        const auto centre = widgetCentre(
            scene.describe(kUiCanvas, antwika::ui::Pointer{}, state), id);

        return Position{
            .x = centre.value_or(antwika::gfx::Point{}).x,
            .y = centre.value_or(antwika::gfx::Point{}).y};
    }

    [[nodiscard]] std::vector<InputEvent> pressOn(Position at)
    {
        return {
            PointerMoved{.position = at},
            PointerButtonPressed{
                .button = MouseButton::Left, .position = at}};
    }

    // One run through the shipped wiring.
    // On a machine whose own layout is the file at optionsPath.
    // A live run reads the device and is told what that file held.
    // A replay reads the events and is told nothing at all.
    // Which is exactly what machineOptionsFor() decides in main().
    [[nodiscard]] Played drive(
        const std::string &optionsPath,
        bool replaying,
        antwika::simulation::ITickEventSource &inner,
        FakeInputBackend &device,
        AppMode start)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        TickEventRecorder recorder;

        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{start};
        antwika::game::PauseState pause;
        UiOverlay overlay(kUiCanvas);
        UiOverlay menuOverlay(kUiCanvas);
        UiOverlay saveOverlay(kUiCanvas);

        const auto machine = machineOptionsFor(replaying, optionsPath);

        InputPipeline input(
            inner,
            device,
            codec,
            InputPipelineOptions{.readsDevice = !replaying});

        BindingSource bound(input, machine.bindings);

        auto summary = antwika::game::bootstrap(
            antwika::game::GameWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = bound,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder,
                .overlay = overlay,
                .menuOverlay = menuOverlay,
                .saveOverlay = saveOverlay,
                .optionsPath = machine.path,
                .translator = kTranslator});

        return Played{
            .summary = std::move(summary),
            .recorded = recorder.getEvents()};
    }

    // What a --record file holds is everything but engine.tick.
    [[nodiscard]] std::vector<TickEvent> fileOf(const Played &run)
    {
        auto events = run.recorded;

        std::erase_if(
            events,
            [](const TickEvent &event)
            {
                return event.event.name
                       == antwika::engine::events::kTick;
            });

        return events;
    }

    [[nodiscard]] std::vector<TickEvent> withoutBindings(
        std::vector<TickEvent> events)
    {
        std::erase_if(
            events,
            [](const TickEvent &event)
            {
                return event.event.name
                       == antwika::game::events::kBindKey;
            });

        return events;
    }

    [[nodiscard]] std::vector<TickEvent> endingAt(antwika::time::Tick tick)
    {
        return {
            TickEvent{
                .tick = tick,
                .event = antwika::event::Event{
                    .name = antwika::engine::events::kStop}}};
    }

    class BindingReplayTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            std::filesystem::create_directories(directory);
            saveOptionsFile(machineOne(), one());
            saveOptionsFile(machineTwo(), two());
        }

        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        [[nodiscard]] std::string one() const
        {
            return (directory / "one.json").string();
        }

        [[nodiscard]] std::string two() const
        {
            return (directory / "two.json").string();
        }

        [[nodiscard]] Played record(
            const std::string &optionsPath,
            std::vector<std::vector<InputEvent>> rounds,
            AppMode start)
        {
            ReplaySource ending(endingAt(kMaxTicks - 2));
            FakeInputBackend device(std::move(rounds));

            return drive(optionsPath, false, ending, device, start);
        }

        [[nodiscard]] Played replay(
            const std::string &optionsPath,
            const std::vector<TickEvent> &file,
            AppMode start)
        {
            ReplaySource fromFile(file);
            FakeInputBackend untouched;

            return drive(optionsPath, true, fromFile, untouched, start);
        }

        std::filesystem::path directory{
            antwika::testing::scratchPath("bindings.")};
    };

    // A session somebody really could have played on machine one.
    // A road laid, and a walker dropped onto it.
    // Then the zoom key and the pause key that machine has.
    // Neither of which means anything at all on machine two.
    [[nodiscard]] std::vector<std::vector<InputEvent>> hotkeySession()
    {
        std::vector<std::vector<InputEvent>> rounds;

        std::vector<InputEvent> laying;
        for (std::int32_t x = 1; x <= 5; ++x)
        {
            laying.push_back(
                PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = atCell(Cell{.x = x, .y = 2})});
        }
        laying.push_back(
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = atCell(Cell{.x = 5, .y = 2})});
        rounds.push_back(std::move(laying));

        rounds.push_back(
            {PointerButtonPressed{
                .button = MouseButton::Right,
                .position = atCell(Cell{.x = 1, .y = 2})}});

        rounds.push_back({KeyPressed{.key = Key::K}});
        rounds.push_back({KeyPressed{.key = Key::J}});
        rounds.push_back({});
        rounds.push_back({});

        return rounds;
    }

    // The other way in.
    // The player rebinds on the options screen and uses the key.
    // N rather than J or K, since a key already held is refused.
    // See BindOutcome::Taken.
    [[nodiscard]] std::vector<std::vector<InputEvent>> rebindSession()
    {
        return {
            pressOn(pixelOnMenu(antwika::game::menuWidgets::kOptions)),
            pressOn(
                pixelOnOptions(
                    antwika::game::optionsWidgets::actionWidget(
                        Action::ZoomIn))),
            {KeyPressed{.key = Key::N}},
            pressOn(pixelOnOptions(antwika::game::optionsWidgets::kBack)),
            pressOn(pixelOnMenu(antwika::game::menuWidgets::kNewGame)),
            {},
            {KeyPressed{.key = Key::N}},
            {}};
    }
} // namespace

// **The claim the whole binding design rests on.**
// A session is recorded on a machine binding zoom K and pause J.
// The file it produces is replayed on one binding them M and L.
// Both reach the same city, the same walkers and the same camera.
// Because the layout the session was played under travels in it.
// Rather than being read off the machine.
TEST_F(BindingReplayTest, ARecordingReplaysOnAMachineBoundDifferently)
{
    const auto recorded =
        record(one(), hotkeySession(), AppMode::CityMap);
    const auto file = fileOf(recorded);

    ASSERT_FALSE(file.empty());

    EXPECT_EQ(replay(two(), file, AppMode::CityMap).summary,
              recorded.summary);
    EXPECT_EQ(replay(one(), file, AppMode::CityMap).summary,
              recorded.summary);
}

// And it may not pass because the keys did nothing.
// Strip the layout out of the file.
// The very same key presses then reach a different city.
// Which is what the file is carrying.
TEST_F(BindingReplayTest, TheLayoutInTheFileIsWhatMakesThatTrue)
{
    const auto recorded =
        record(one(), hotkeySession(), AppMode::CityMap);
    const auto stripped = withoutBindings(fileOf(recorded));

    EXPECT_NE(replay(two(), stripped, AppMode::CityMap).summary,
              recorded.summary);
}

// The layout really did travel.
// Rather than both runs quietly falling back on what ships.
TEST_F(BindingReplayTest, TheRecordingCarriesTheMachinesOwnLayout)
{
    const auto recorded =
        record(one(), hotkeySession(), AppMode::CityMap);

    EXPECT_EQ(recorded.summary.bindings, machineOne());
    EXPECT_NE(recorded.summary.bindings, kDefaultBindings);

    const auto file = fileOf(recorded);
    EXPECT_NE(
        std::count_if(
            file.begin(),
            file.end(),
            [](const TickEvent &event)
            {
                return event.event.name
                       == antwika::game::events::kBindKey;
            }),
        0);
}

// The other half of the feature, end to end through the front door.
// Open the options, pick an action, press a key, leave and play.
// None of that is an event of its own.
// The recording holds the clicks and the key presses.
// And the replay works the binding out again.
TEST_F(BindingReplayTest, ARebindingMadeOnTheScreenReplaysAsItself)
{
    const auto recorded =
        record(one(), rebindSession(), AppMode::MainMenu);

    // Bound on the screen, over what the machine came with.
    EXPECT_EQ(recorded.summary.bindings.keyFor(Action::ZoomIn), Key::N);

    // And it was used: the camera moved.
    EXPECT_NE(recorded.summary.camera, Camera());

    const auto replayed =
        replay(two(), fileOf(recorded), AppMode::MainMenu);

    EXPECT_EQ(replayed.summary, recorded.summary);
}

// **A replay leaves the machine it is replayed on alone.**
// It was told nothing about that machine's layout.
// So it may not leave it carrying the session's.
TEST_F(BindingReplayTest, AReplayDoesNotRebindTheMachineItRunsOn)
{
    const auto recorded =
        record(one(), rebindSession(), AppMode::MainMenu);

    (void)replay(two(), fileOf(recorded), AppMode::MainMenu);

    EXPECT_EQ(
        antwika::game::loadOptionsFileOrDefaults(two()), machineTwo());
}

// And a live run leaves what it ended with.
// Where the next one will find it.
TEST_F(BindingReplayTest, ALiveRunLeavesItsLayoutOnTheMachine)
{
    const auto recorded =
        record(one(), rebindSession(), AppMode::MainMenu);

    EXPECT_EQ(
        antwika::game::loadOptionsFileOrDefaults(one()),
        recorded.summary.bindings);
}
