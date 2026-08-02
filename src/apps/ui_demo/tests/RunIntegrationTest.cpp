#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/TickBudgetSource.hpp"
#include "antwika/ui_demo/UiDemo.hpp"
#include "antwika/ui_demo/Widgets.hpp"
#include "WidgetCentre.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::ui_demo::DemoOverlay;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::DemoSummary;
using antwika::ui_demo::Showcase;
using antwika::ui_demo::TickBudgetSource;
using antwika::ui_demo::UiDemoConfig;
using antwika::ui_demo::tests::optionWidget;
using antwika::ui_demo::tests::widgetCentre;
using ::testing::NiceMock;
namespace widgets = antwika::ui_demo::widgets;

namespace
{
    // The locale is a constant of the build, so a test may name one.
    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr antwika::time::Tick kBudget = 6;
    constexpr antwika::time::Tick kMaxTicks = 30;

    [[nodiscard]] Point centreOn(
        const DemoState &state, const antwika::ui::WidgetId id)
    {
        const DemoScene scene{kTranslator};
        const auto found =
            widgetCentre(scene.describe(kCanvas, {}, {}, state), id);
        EXPECT_TRUE(found.has_value());
        return found.value_or(Point{});
    }

    [[nodiscard]] TickEvent pressAt(
        const InputEventCodec &codec,
        const Point at,
        const antwika::time::Tick when)
    {
        return TickEvent{
            .tick = when,
            .event = codec.encode(PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}})};
    }

    // Open the picker, choose the buttons page, press count twice.
    // Every position is read off the layout rather than guessed at.
    // Which is the same thing ui::Frame::rects does for the app.
    [[nodiscard]] std::vector<TickEvent> script()
    {
        const InputEventCodec codec;

        DemoState closed;
        DemoState opened;
        opened.setPickerOpen(true);
        DemoState buttons;
        buttons.select(static_cast<std::size_t>(Showcase::Buttons));

        const auto option = optionWidget(
            widgets::kFirstPage,
            static_cast<std::uint64_t>(Showcase::Buttons));

        return {
            pressAt(codec, centreOn(closed, widgets::kPicker), 1),
            pressAt(codec, centreOn(opened, option), 2),
            pressAt(codec, centreOn(buttons, widgets::kCount), 3),
            pressAt(codec, centreOn(buttons, widgets::kCount), 4)};
    }

    // Enough characters that the name outgrows a short string.
    // A shorter run leaves DemoSummary's copy on one path only.
    constexpr std::size_t kTypedCharacters = 20;
    constexpr antwika::time::Tick kTypingBudget = kTypedCharacters + 4;

    // Choose the text field's page, press the field, then type.
    // Nothing here says "the field now holds this".
    // The key press is recorded and the characters worked out again.
    [[nodiscard]] std::vector<TickEvent> typingScript()
    {
        const InputEventCodec codec;

        DemoState opened;
        opened.setPickerOpen(true);
        DemoState field;
        field.select(static_cast<std::size_t>(Showcase::TextField));

        const auto option = optionWidget(
            widgets::kFirstPage,
            static_cast<std::uint64_t>(Showcase::TextField));

        std::vector<TickEvent> script{
            pressAt(codec, centreOn(DemoState{}, widgets::kPicker), 1),
            pressAt(codec, centreOn(opened, option), 2),
            pressAt(codec, centreOn(field, widgets::kField), 3)};

        for (std::size_t index = 0; index < kTypedCharacters; ++index)
        {
            script.push_back(TickEvent{
                .tick = 4 + index,
                .event = codec.encode(KeyPressed{
                    .key = antwika::input::Key::A, .modifiers = {}})});
        }

        return script;
    }

    // What the watcher below saw, kept outside it.
    // bootstrap() owns the sink and destroys it before returning.
    struct WatchedTicks
    {
        std::uint64_t ticks = 0;
        std::size_t commands = 0;
        std::string text;
    };

    // Stands in for the renderer main.cpp hands bootstrap().
    // It reads exactly what RenderSink reads.
    // So what it sees is what a frame would have been drawn from.
    class FinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FinishedTickWatcher(
            const DemoState &state,
            const DemoOverlay &overlay,
            WatchedTicks &seen)
            : state(state), overlay(overlay), seen(seen)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }

            ++seen.ticks;
            seen.commands = overlay.commands().size();
            seen.text = state.text();
        }

    private:
        const DemoState &state;
        const DemoOverlay &overlay;
        WatchedTicks &seen;
    };

    TEST(RunIntegrationTest, Bootstrap_ReplaysClicksIntoPagesAndCounts)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        const InputEventCodec codec;

        ReplaySource file(script());
        TickBudgetSource source(file, kBudget);

        WatchedTicks seen;

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .extraSink =
                [&seen](
                    const DemoState &state, const DemoOverlay &overlay)
            {
                return std::make_unique<FinishedTickWatcher>(
                    state, overlay, seen);
            }});

        EXPECT_EQ(summary.showcase, Showcase::Buttons);
        EXPECT_EQ(summary.clicks, 2U);
        EXPECT_TRUE(seen.text.empty());
        EXPECT_GT(summary.commands, 0U);

        // The budget's tick still runs to completion.
        EXPECT_EQ(seen.ticks, kBudget + 1);
        EXPECT_GT(seen.commands, 0U);
    }

    TEST(RunIntegrationTest, Bootstrap_RecordsTheInputItWasGiven)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        const InputEventCodec codec;

        ReplaySource file(script());
        TickBudgetSource source(file, kBudget);
        TickEventRecorder recorder;

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .replayRecorder = recorder});

        EXPECT_EQ(summary.clicks, 2U);

        // Only what came in from outside, never a ui.* name.
        for (const auto &event : recorder.getEvents())
        {
            EXPECT_EQ(event.event.name.rfind("ui.", 0), std::string::npos);
        }

        EXPECT_FALSE(recorder.getEvents().empty());
    }

    TEST(RunIntegrationTest, Bootstrap_RegeneratesTypingFromKeyPresses)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        const InputEventCodec codec;

        ReplaySource file(typingScript());
        TickBudgetSource source(file, kTypingBudget);

        WatchedTicks seen;

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .extraSink =
                [&seen](
                    const DemoState &state, const DemoOverlay &overlay)
            {
                return std::make_unique<FinishedTickWatcher>(
                    state, overlay, seen);
            }});

        EXPECT_EQ(summary.showcase, Showcase::TextField);
        EXPECT_EQ(seen.text, std::string(kTypedCharacters, 'a'));
    }

    TEST(RunIntegrationTest, Bootstrap_RunsWithNothingDrawingAtAll)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> events;
        const InputEventCodec codec;

        ReplaySource file({});
        TickBudgetSource source(file, 0);

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = events,
            .inputSource = source,
            .codec = codec,
            .translator = kTranslator,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks});

        EXPECT_EQ(summary.showcase, Showcase::Labels);
        EXPECT_EQ(summary.clicks, 0U);
    }
} // namespace
