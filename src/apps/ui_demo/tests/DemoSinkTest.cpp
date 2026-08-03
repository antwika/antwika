#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetCentre.hpp"
#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoSink.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/Widgets.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::ui::WidgetId;
using antwika::ui_demo::DemoOverlay;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::DemoSink;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::MessageId;
using antwika::ui_demo::Showcase;
using antwika::ui_demo::tests::optionWidget;
using antwika::ui_demo::tests::widgetCentre;
namespace widgets = antwika::ui_demo::widgets;

namespace
{
    // The locale is a constant of the build, so a test may name one.
    // Every case here asserts the English showcase's own layout.
    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};

    TickEvent tick(const antwika::time::Tick at = 0)
    {
        return TickEvent{
            .tick = at,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

    TickEvent pressAt(
        const InputEventCodec &codec,
        const Point at,
        const MouseButton button = MouseButton::Left,
        const antwika::time::Tick when = 0)
    {
        return TickEvent{
            .tick = when,
            .event = codec.encode(PointerButtonPressed{
                .button = button,
                .position = {.x = at.x, .y = at.y}})};
    }

    TickEvent moveTo(
        const InputEventCodec &codec,
        const Point at,
        const antwika::time::Tick when = 0)
    {
        return TickEvent{
            .tick = when,
            .event = codec.encode(
                PointerMoved{.position = {.x = at.x, .y = at.y}})};
    }

    TickEvent keyDown(
        const InputEventCodec &codec,
        const Key key,
        const bool shift = false,
        const antwika::time::Tick when = 0)
    {
        return TickEvent{
            .tick = when,
            .event = codec.encode(
                KeyPressed{.key = key, .modifiers = {.shift = shift}})};
    }

    /**
     * @brief One sink and everything it drives, wired as bootstrap()
     * wires them.
     */
    struct Wiring
    {
        DemoState state;
        DemoOverlay overlay{kCanvas};
        InputEventCodec codec;
        DemoScene scene{kTranslator};
        DemoSink sink{state, overlay, codec, scene};

        [[nodiscard]] Point centreOf(const WidgetId id)
        {
            const auto frame = scene.describe(kCanvas, {}, {}, state);
            const auto found = widgetCentre(frame, id);
            EXPECT_TRUE(found.has_value());
            return found.value_or(Point{});
        }

        // The whole rectangle, for a test that aims off-centre.
        [[nodiscard]] antwika::gfx::Rect rectOf(const WidgetId id)
        {
            const auto frame = scene.describe(kCanvas, {}, {}, state);
            const auto found = frame.rects.find(id);
            EXPECT_TRUE(found.has_value());
            return found.value_or(antwika::gfx::Rect{});
        }

        void show(const Showcase page)
        {
            state.select(static_cast<std::size_t>(page));
        }
    };

    TEST(DemoSinkTest, Handle_DescribesThePictureOnATick)
    {
        Wiring wiring;

        wiring.sink.handle(other());
        EXPECT_TRUE(wiring.overlay.commands().empty());

        wiring.sink.handle(tick());
        EXPECT_FALSE(wiring.overlay.commands().empty());
    }

    TEST(DemoSinkTest, Handle_OpensThePickerWhenItIsPressed)
    {
        Wiring wiring;

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kPicker)));
        EXPECT_TRUE(wiring.state.pickerOpen());

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kPicker)));
        EXPECT_FALSE(wiring.state.pickerOpen());
    }

    TEST(DemoSinkTest, Handle_ShowsThePageAnOptionNames)
    {
        Wiring wiring;
        wiring.state.setPickerOpen(true);

        const auto option = optionWidget(
            widgets::kFirstPage,
            static_cast<std::uint64_t>(Showcase::Shrink));

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(option)));

        EXPECT_EQ(wiring.state.showcase(), Showcase::Shrink);
        EXPECT_FALSE(wiring.state.pickerOpen());
        ASSERT_TRUE(wiring.state.message().has_value());
        EXPECT_EQ(
            wiring.state.message()->id, MessageId::Showing);
        EXPECT_EQ(
            wiring.state.message()->argId,
            MessageId::PageShrink);
    }

    TEST(DemoSinkTest, Handle_CountsAndResetsTheCountingButton)
    {
        Wiring wiring;
        wiring.show(Showcase::Buttons);

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kCount)));
        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kCount)));
        EXPECT_EQ(wiring.state.clicks(), 2U);

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kReset)));
        EXPECT_EQ(wiring.state.clicks(), 0U);
    }

    TEST(DemoSinkTest, Handle_OpensTheAccentListAndTakesAChoice)
    {
        Wiring wiring;
        wiring.show(Showcase::Dropdown);

        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(widgets::kPalette)));
        EXPECT_TRUE(wiring.state.accentOpen());

        const auto option = optionWidget(widgets::kFirstAccent, 2);
        wiring.sink.handle(
            pressAt(wiring.codec, wiring.centreOf(option)));

        EXPECT_EQ(wiring.state.accent(), 2U);
        EXPECT_FALSE(wiring.state.accentOpen());
        ASSERT_TRUE(wiring.state.message().has_value());
        EXPECT_EQ(
            wiring.state.message()->id,
            MessageId::AccentChosen);
        EXPECT_EQ(wiring.state.message()->datum, "2");
    }

    TEST(DemoSinkTest, Handle_ReportsAPressOnAnyOtherNamedWidget)
    {
        Wiring wiring;
        wiring.show(Showcase::TextField);

        const auto at = wiring.centreOf(widgets::kField);
        wiring.sink.handle(pressAt(wiring.codec, at));

        EXPECT_EQ(wiring.state.focus(), widgets::kField);
        ASSERT_TRUE(wiring.state.message().has_value());
        EXPECT_EQ(
            wiring.state.message()->id,
            MessageId::PressedWidget);
        EXPECT_EQ(wiring.state.message()->datum, "5");
    }

    TEST(DemoSinkTest, Handle_LeavesEverythingWhereAPressHitsNothing)
    {
        Wiring wiring;

        // Below the page's own panel, where nothing is named.
        const auto frame =
            wiring.scene.describe(kCanvas, {}, {}, wiring.state);
        const auto card = frame.rects.find(widgets::kCard);
        ASSERT_TRUE(card.has_value());

        wiring.sink.handle(pressAt(
            wiring.codec,
            Point{
                .x = static_cast<std::int32_t>(kCanvas.width) - 1,
                .y = card->origin.y
                     + static_cast<std::int32_t>(card->size.height)
                     + 1}));

        EXPECT_EQ(wiring.state.focus(), antwika::ui::kNoWidget);
        EXPECT_FALSE(wiring.state.message().has_value());
    }

    TEST(DemoSinkTest, Handle_OnlyALeftPressActivatesAnything)
    {
        Wiring wiring;
        wiring.show(Showcase::Buttons);

        wiring.sink.handle(pressAt(
            wiring.codec,
            wiring.centreOf(widgets::kCount),
            MouseButton::Right));

        EXPECT_EQ(wiring.state.clicks(), 0U);
    }

    TEST(DemoSinkTest, Handle_TypesIntoTheFocusedField)
    {
        Wiring wiring;
        wiring.show(Showcase::TextField);
        wiring.state.setFocus(widgets::kField);

        wiring.sink.handle(keyDown(wiring.codec, Key::H, true));
        wiring.sink.handle(keyDown(wiring.codec, Key::I));

        EXPECT_EQ(wiring.state.text(), "Hi");
        EXPECT_EQ(wiring.state.caret(), 2U);
    }

    // The area is the second editable widget, told apart by id.
    TEST(DemoSinkTest, Handle_TypesIntoTheFocusedArea)
    {
        Wiring wiring;
        wiring.show(Showcase::TextArea);
        wiring.state.setFocus(widgets::kArea);
        wiring.state.setArea("ab", 2, 2);

        wiring.sink.handle(keyDown(wiring.codec, Key::C));

        EXPECT_EQ(wiring.state.areaText(), "abc");
        EXPECT_EQ(wiring.state.areaCursor(), 3U);
        EXPECT_EQ(wiring.state.areaAnchor(), 3U);
    }

    // A held drag that began in the text keeps selecting as it moves.
    // The press lays the caret and the anchor together.
    // The motion then pulls the cursor away, which is a selection.
    TEST(DemoSinkTest, Handle_DragsASelectionAcrossTheArea)
    {
        Wiring wiring;
        wiring.show(Showcase::TextArea);
        wiring.state.setFocus(widgets::kArea);
        wiring.state.setArea("0123456789 0123456789 0123456789", 0, 0);

        // A point on the first text row, a few characters in.
        const auto rect = wiring.rectOf(widgets::kArea);
        const auto inText = Point{
            .x = rect.origin.x + 45, .y = rect.origin.y + 30};

        wiring.sink.handle(pressAt(wiring.codec, inText));

        const auto laid = wiring.state.areaCursor();

        EXPECT_EQ(laid, wiring.state.areaAnchor());
        EXPECT_GT(laid, 0U);

        wiring.sink.handle(moveTo(
            wiring.codec,
            Point{.x = inText.x + 60, .y = inText.y},
            1));

        EXPECT_EQ(wiring.state.areaAnchor(), laid);
        EXPECT_GT(wiring.state.areaCursor(), laid);
    }

    // A drag that began on the track keeps scrolling as it moves.
    // Before the drag was scoped, the thumb stuck after the press.
    TEST(DemoSinkTest, Handle_DragsTheScrollThumbAcrossTheTrack)
    {
        Wiring wiring;
        wiring.show(Showcase::TextArea);

        std::string lines;

        for (int line = 0; line < 40; ++line)
        {
            lines += "line\n";
        }

        wiring.state.setArea(lines, 0, 0);

        // A point on the track, low down it.
        const auto rect = wiring.rectOf(widgets::kArea);

        const auto trackX = rect.origin.x
                            + static_cast<std::int32_t>(rect.size.width)
                            - 22;

        wiring.sink.handle(pressAt(
            wiring.codec,
            Point{.x = trackX, .y = rect.origin.y + 95}));

        const auto jumped = wiring.state.areaScroll();

        EXPECT_GT(jumped, 0U);

        // Held, back up the track: the scroll follows the motion.
        wiring.sink.handle(moveTo(
            wiring.codec,
            Point{.x = trackX, .y = rect.origin.y + 35},
            1));

        EXPECT_LT(wiring.state.areaScroll(), jumped);
        EXPECT_GT(wiring.state.areaScroll(), 0U);
    }

    // The Interactions::scrolled round trip, through the sink.
    // Asking for a line past the end comes back clamped, once.
    TEST(DemoSinkTest, Handle_TakesTheLineThePaneSaysItIsShowing)
    {
        Wiring wiring;
        wiring.show(Showcase::TextArea);
        wiring.state.setAreaScroll(500);

        wiring.sink.handle(tick());

        EXPECT_LT(wiring.state.areaScroll(), 500U);

        // Handed back, the report settles: a second frame is quiet.
        const auto settled = wiring.state.areaScroll();

        wiring.sink.handle(tick(1));

        EXPECT_EQ(wiring.state.areaScroll(), settled);
    }

    TEST(DemoSinkTest, Handle_SubmitsTheFieldOnEnter)
    {
        Wiring wiring;
        wiring.show(Showcase::TextField);
        wiring.state.setFocus(widgets::kField);
        wiring.state.setText("ok", 2);

        wiring.sink.handle(keyDown(wiring.codec, Key::Enter));

        ASSERT_TRUE(wiring.state.message().has_value());
        EXPECT_EQ(
            wiring.state.message()->id, MessageId::Submitted);
        EXPECT_EQ(wiring.state.message()->datum, "ok");
        EXPECT_EQ(wiring.state.text(), "ok");
    }

    TEST(DemoSinkTest, Handle_EmptiesTheFieldOnEscape)
    {
        Wiring wiring;
        wiring.show(Showcase::TextField);
        wiring.state.setFocus(widgets::kField);
        wiring.state.setText("gone", 4);

        wiring.sink.handle(keyDown(wiring.codec, Key::Escape));

        EXPECT_TRUE(wiring.state.text().empty());
        ASSERT_TRUE(wiring.state.message().has_value());
        EXPECT_EQ(
            wiring.state.message()->id, MessageId::Cancelled);
    }

    TEST(DemoSinkTest, Handle_WalksTheFocusRingWithTabAndShiftTab)
    {
        Wiring wiring;
        wiring.show(Showcase::Focus);

        wiring.sink.handle(keyDown(wiring.codec, Key::Tab));
        EXPECT_EQ(wiring.state.focus(), widgets::kPicker);

        wiring.sink.handle(keyDown(wiring.codec, Key::Tab));
        EXPECT_EQ(wiring.state.focus(), widgets::kFirst);

        wiring.sink.handle(keyDown(wiring.codec, Key::Tab, true));
        EXPECT_EQ(wiring.state.focus(), widgets::kPicker);
    }

    TEST(DemoSinkTest, Handle_IgnoresAKeyTheUiHasNoMeaningFor)
    {
        Wiring wiring;
        wiring.show(Showcase::TextField);
        wiring.state.setFocus(widgets::kField);

        wiring.sink.handle(keyDown(wiring.codec, Key::F1));

        EXPECT_TRUE(wiring.state.text().empty());
        EXPECT_EQ(wiring.state.focus(), widgets::kField);
    }

    TEST(DemoSinkTest, Handle_ClearsAnEdgeOnTheNextTicksFirstEvent)
    {
        Wiring wiring;
        wiring.show(Showcase::Buttons);

        const auto at = wiring.centreOf(widgets::kCount);

        // Two events inside one tick, only one of them a press.
        wiring.sink.handle(pressAt(wiring.codec, at));
        wiring.sink.handle(moveTo(wiring.codec, at));
        EXPECT_EQ(wiring.state.clicks(), 1U);

        // And a new tick, which is what clears the fold's edges.
        wiring.sink.handle(moveTo(wiring.codec, at, 1));
        EXPECT_EQ(wiring.state.clicks(), 1U);
    }
} // namespace
