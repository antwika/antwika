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


#include "Translators.hpp"
#include "WidgetCentre.hpp"
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
    constexpr auto kLocale = antwika::i18n::kDefaultLocale;
    constexpr Size kCanvas{.width = 1024, .height = 640};

    constexpr Position kOffEveryItem{.x = 2, .y = 2};

    class MainMenuSinkTest : public ::testing::Test
    {
    protected:
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
}

TEST_F(MainMenuSinkTest, Handle_DescribesTheMenuEveryTick)
{
    EXPECT_TRUE(overlay.commands().empty());

    tick();

    EXPECT_FALSE(overlay.commands().empty());
}

TEST_F(MainMenuSinkTest, Handle_AsksForPlayingOnNewGame)
{
    pressAt(pixelOn(menuWidgets::kNewGame));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::CityMap);

    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}

TEST_F(MainMenuSinkTest, Handle_PressingQuitStopsTheRun)
{
    EXPECT_FALSE(stop.stopped());

    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(stop.stopped());
}

TEST_F(MainMenuSinkTest, Handle_ChangesNothingOnAMissedPress)
{
    pressAt(kOffEveryItem);

    EXPECT_EQ(mode.next(), AppMode::MainMenu);
    EXPECT_FALSE(stop.stopped());
}

TEST_F(MainMenuSinkTest, Handle_AsksForTheWorldMapMode)
{
    pressAt(pixelOn(menuWidgets::kWorldMap));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::WorldMap);
}

TEST_F(MainMenuSinkTest, Handle_AsksForTheSaveScreenOnLoad)
{
    pressAt(pixelOn(menuWidgets::kLoadGame));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::SaveLoad);
}

TEST_F(MainMenuSinkTest, Handle_EveryItemIsReachableAndDistinct)
{
    const auto newGame = pixelOn(menuWidgets::kNewGame);
    const auto load = pixelOn(menuWidgets::kLoadGame);
    const auto world = pixelOn(menuWidgets::kWorldMap);
    const auto quit = pixelOn(menuWidgets::kQuit);

    EXPECT_LT(newGame.y, load.y);
    EXPECT_LT(load.y, world.y);
    EXPECT_LT(world.y, quit.y);
}

TEST_F(MainMenuSinkTest, Handle_DoesNothingOutsideTheMainMenu)
{
    mode.request(AppMode::CityMap);
    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});

    tick();
    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(stop.stopped());
}

TEST_F(MainMenuSinkTest, Handle_AnEventThatIsNotInputIsIgnored)
{
    dispatch(TickEvent{
        .tick = 0, .event = Event{.name = "game.score_increment"}});

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

TEST_F(MainMenuSinkTest, Handle_AMiddlePressActivatesNothing)
{
    const auto at = pixelOn(menuWidgets::kQuit);

    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle, .position = at});

    EXPECT_FALSE(stop.stopped());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

TEST_F(MainMenuSinkTest, Handle_ShowsTheKeyBindingsOnOptions)
{
    const auto at = pixelOn(menuWidgets::kOptions);

    pressAt(at);

    EXPECT_TRUE(options.open());

    EXPECT_EQ(mode.next(), AppMode::MainMenu);

    tick();

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

TEST_F(MainMenuSinkTest, Handle_TheOptionsScreenIsLeavable)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::kBack));

    EXPECT_FALSE(options.open());

    tick();
    EXPECT_EQ(
        overlay.commands(), scene.describe(kCanvas, Pointer{}).commands);
}

TEST_F(MainMenuSinkTest, Handle_TakesTheNextKeyForAnAction)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::actionWidget(Action::Pause)));
    EXPECT_EQ(options.awaiting(), Action::Pause);

    send(KeyPressed{.key = Key::J});

    EXPECT_EQ(options.bindings().keyFor(Action::Pause), Key::J);
    EXPECT_FALSE(options.awaiting().has_value());
}

TEST_F(MainMenuSinkTest, Handle_DoesNotOfferAHeldKeyTwice)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(optionsWidgets::actionWidget(Action::Pause)));

    send(KeyPressed{.key = Key::J, .repeat = true});

    EXPECT_EQ(options.bindings(), antwika::game::kDefaultBindings);
    EXPECT_EQ(options.awaiting(), Action::Pause);
}

TEST_F(MainMenuSinkTest, Handle_APressOnNoRowAsksForNothing)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(kOffEveryItem);

    EXPECT_TRUE(options.open());
    EXPECT_FALSE(options.awaiting().has_value());
}

TEST_F(MainMenuSinkTest, Handle_StagesAPickedLanguage)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(pixelOnOptions(
        optionsWidgets::languageWidget(antwika::i18n::Locale::Swedish)));

    EXPECT_EQ(localeState.locale(), kLocale);
    EXPECT_EQ(localeState.next(), antwika::i18n::Locale::Swedish);

    localeState.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(localeState.locale(), antwika::i18n::Locale::Swedish);
}

TEST_F(MainMenuSinkTest, Handle_RemembersAPickedLanguage)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    EXPECT_EQ(options.locale(), kLocale);

    pressAt(pixelOnOptions(
        optionsWidgets::languageWidget(antwika::i18n::Locale::Swedish)));

    EXPECT_EQ(options.locale(), antwika::i18n::Locale::Swedish);
}

TEST_F(MainMenuSinkTest, Handle_APressOnNoLanguageChangesNothing)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    pressAt(kOffEveryItem);

    EXPECT_EQ(localeState.next(), kLocale);
    EXPECT_EQ(options.locale(), kLocale);
}

TEST_F(MainMenuSinkTest, Handle_TakesAPickedLayoutAtOnce)
{
    pressAt(pixelOn(menuWidgets::kOptions));
    tick();

    EXPECT_EQ(
        options.keyboard(), antwika::game::kDefaultKeyboardLayout);

    pressAt(pixelOnOptions(optionsWidgets::keyboardWidget(
        antwika::game::KeyboardLayout::English)));

    EXPECT_EQ(
        options.keyboard(), antwika::game::KeyboardLayout::English);
}
