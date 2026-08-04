#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetPixel.hpp"

#include "TestTranslator.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MainMenuSink.hpp"
#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::game::tests::kLanguages;
using antwika::game::tests::kTranslator;

using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::InputFold;
using antwika::game::MainMenuScene;
using antwika::game::MainMenuSink;
using antwika::game::Action;
using antwika::game::OptionsScene;
using antwika::game::OptionsState;
using antwika::game::UiOverlay;
using antwika::game::tests::widgetCentre;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace menuWidgets = antwika::game::menuWidgets;
namespace optionsWidgets = antwika::game::optionsWidgets;

namespace
{
    // The language every layout in this file is described in.
    // Named once so a call to describe() still fits a line.
    // And named rather than defaulted on purpose.
    // Which language a layout was built in is what these rest on.
    constexpr auto kLocale = antwika::i18n::kDefaultLocale;
    constexpr Size kCanvas{.width = 1024, .height = 640};

    // The corner: the menu's card is centred, so nothing is there.
    constexpr Position kOffEveryItem{.x = 2, .y = 2};

    class MainMenuSinkTest : public ::testing::Test
    {
    protected:
        // Where an item is, is the layout's business.
        // So a test asks the layout for the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            const auto centre =
                widgetCentre(scene.describe(kCanvas, Pointer{}), id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        [[nodiscard]] Position pixelOnOptions(WidgetId id) const
        {
            const auto centre = widgetCentre(
                optionsScene.describe(
                    kCanvas, Pointer{}, options, kLocale),
                id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        // Through the fold first, as bootstrap() registers it.
        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void pressAt(Position at)
        {
            send(PointerMoved{.position = at});
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = at});
        }

        void tick()
        {
            dispatch(
                TickEvent{
                    .tick = 0,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        InputEventCodec codec;
        AppModeState mode;
        UiOverlay overlay{kCanvas};
        InputFold input{codec};
        MainMenuScene scene{kTranslator};
        StopSignal stop;
        OptionsState options;
        OptionsScene optionsScene{kTranslator, kLanguages};
        antwika::game::LocaleState localeState{};

        MainMenuSink sink{
            mode,
            overlay,
            input,
            scene,
            stop,
            options,
            optionsScene,
            localeState};
    };
} // namespace

TEST_F(MainMenuSinkTest, TheMenuIsDescribedForTheRendererEveryTick)
{
    EXPECT_TRUE(overlay.commands().empty());

    tick();

    EXPECT_FALSE(overlay.commands().empty());
}

TEST_F(MainMenuSinkTest, PressingNewGameAsksForThePlayingMode)
{
    pressAt(pixelOn(menuWidgets::kNewGame));

    // Staged, not applied: the same click must not also reach the grid.
    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::CityMap);

    // And applied at the boundary, which the state itself owns.
    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}

TEST_F(MainMenuSinkTest, PressingQuitStopsTheRun)
{
    EXPECT_FALSE(stop.stopped());

    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(stop.stopped());
}

TEST_F(MainMenuSinkTest, APressThatMissesEveryItemChangesNothing)
{
    pressAt(kOffEveryItem);

    EXPECT_EQ(mode.next(), AppMode::MainMenu);
    EXPECT_FALSE(stop.stopped());
}

TEST_F(MainMenuSinkTest, PressingWorldMapAsksForTheWorldMapMode)
{
    pressAt(pixelOn(menuWidgets::kWorldMap));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::WorldMap);
}

TEST_F(MainMenuSinkTest, PressingLoadGameAsksForTheSaveScreen)
{
    pressAt(pixelOn(menuWidgets::kLoadGame));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::SaveLoad);
}

// Every item now names a widget, and each names its own.
// A press walking down the card therefore hits four different ones.
TEST_F(MainMenuSinkTest, EveryItemIsReachableAndDistinct)
{
    const auto newGame = pixelOn(menuWidgets::kNewGame);
    const auto load = pixelOn(menuWidgets::kLoadGame);
    const auto world = pixelOn(menuWidgets::kWorldMap);
    const auto quit = pixelOn(menuWidgets::kQuit);

    EXPECT_LT(newGame.y, load.y);
    EXPECT_LT(load.y, world.y);
    EXPECT_LT(world.y, quit.y);
}

TEST_F(MainMenuSinkTest, NothingHappensOutsideTheMainMenuMode)
{
    mode.request(AppMode::CityMap);
    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});

    tick();
    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(stop.stopped());
}

// A non-input event carries nothing the fold could hand over.
TEST_F(MainMenuSinkTest, AnEventThatIsNotInputIsIgnored)
{
    dispatch(TickEvent{
        .tick = 0, .event = Event{.name = "game.score_increment"}});

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

// A press is the left button's, and only the left button's.
TEST_F(MainMenuSinkTest, AMiddlePressActivatesNothing)
{
    const auto at = pixelOn(menuWidgets::kQuit);

    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle, .position = at});

    EXPECT_FALSE(stop.stopped());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

// The options screen is this screen with something else on it.
// So one overlay, one scene to paint it, and a flag saying which.
TEST_F(MainMenuSinkTest, PressingOptionsShowsTheKeyBindings)
{
    const auto at = pixelOn(menuWidgets::kOptions);

    pressAt(at);

    EXPECT_TRUE(options.open());

    // And it stays on this screen rather than becoming a mode.
    EXPECT_EQ(mode.next(), AppMode::MainMenu);

    tick();

    // Described at the pointer the sink actually has.
    // Still where the press left it, and still held.
    // Whatever it lands on is drawn hovered.
    // So comparing against an empty pointer agrees only by luck.
    // That is a fact about the card's height, not about this sink.
    EXPECT_EQ(
        overlay.commands(),
        optionsScene
            .describe(
                kCanvas,
                Pointer{
                    .position = Point{.x = at.x, .y = at.y},
                    .down = true},
                options,
                kLocale)
            .commands);
}

TEST_F(MainMenuSinkTest, TheOptionsScreenIsLeavable)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::kBack));

    EXPECT_FALSE(options.open());

    tick();
    EXPECT_EQ(
        overlay.commands(), scene.describe(kCanvas, Pointer{}).commands);
}

// Pick an action, press a key, and that is what the key means.
// None of it is an event of its own.
// The recording holds the click and the press.
// And a replay works the binding out again.
TEST_F(MainMenuSinkTest, AnActionTakesTheNextKeyPressed)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::actionWidget(Action::Pause)));
    EXPECT_EQ(options.awaiting(), Action::Pause);

    send(KeyPressed{.key = Key::J});

    EXPECT_EQ(options.bindings().keyFor(Action::Pause), Key::J);
    EXPECT_FALSE(options.awaiting().has_value());
}

// A repeat is not a fresh press.
// So holding a key down offers it once and no more.
TEST_F(MainMenuSinkTest, AHeldKeyIsNotOfferedTwice)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::actionWidget(Action::Pause)));

    send(KeyPressed{.key = Key::J, .repeat = true});

    EXPECT_EQ(options.bindings(), antwika::game::kDefaultBindings);
    EXPECT_EQ(options.awaiting(), Action::Pause);
}

// A press on the screen's own card that lands on no row at all.
TEST_F(MainMenuSinkTest, APressOnNoRowAsksForNothing)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(kOffEveryItem);

    EXPECT_TRUE(options.open());
    EXPECT_FALSE(options.awaiting().has_value());
}

// The press is recorded; the change is worked out from it again.
// So this stages rather than writing anything to the wire.
TEST_F(MainMenuSinkTest, PressingALanguageStagesItForTheNextTick)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(
        optionsWidgets::languageWidget(antwika::i18n::Locale::Swedish)));

    EXPECT_EQ(localeState.locale(), kLocale);
    EXPECT_EQ(localeState.next(), antwika::i18n::Locale::Swedish);

    // And it lands at the boundary the state itself owns.
    localeState.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(localeState.locale(), antwika::i18n::Locale::Swedish);
}

// The preference is written down too.
// So the file carries it and the next live run announces it.
// See LocaleSource.
TEST_F(MainMenuSinkTest, PressingALanguageAlsoRemembersThePreference)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    EXPECT_EQ(options.locale(), kLocale);

    pressAt(pixelOnOptions(
        optionsWidgets::languageWidget(antwika::i18n::Locale::Swedish)));

    EXPECT_EQ(options.locale(), antwika::i18n::Locale::Swedish);
}

TEST_F(MainMenuSinkTest, APressOnNoLanguageChangesNothing)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(kOffEveryItem);

    EXPECT_EQ(localeState.next(), kLocale);
    EXPECT_EQ(options.locale(), kLocale);
}
