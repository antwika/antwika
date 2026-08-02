#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetPixel.hpp"

#include "FakeMenuCommands.hpp"
#include "TestTranslator.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IMenuCommands.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/UiSink.hpp"

using antwika::game::tests::kTranslator;

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::InputFold;
using antwika::game::MenuItem;
using antwika::game::MenuModalScene;
using antwika::game::PauseState;
using antwika::game::RoadDrag;
using antwika::game::Toolbar;
using antwika::game::UiOverlay;
using antwika::game::UiSink;
using antwika::game::tests::widgetCentre;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace widgets = antwika::game::widgets;
namespace modalWidgets = antwika::game::modalWidgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};
    constexpr Point kHome{.x = 512, .y = 48};

    // Between the two strips and left of the palette, so the city.
    // ToolbarTest is where that is asserted off the layout.
    // Left of the modal's card as well.
    // So a press here is on the scrim rather than on an item.
    constexpr Position kOnTheGrid{.x = 200, .y = 300};

    [[nodiscard]] std::vector<std::string> textsOf(const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    class UiSinkTest : public ::testing::Test
    {
    protected:
        // Where a button is, is the layout's business.
        // So a test asks the layout for the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            const auto centre = widgetCentre(
                toolbar.describe(kCanvas, Pointer{}, camera), id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        // The list exists only while it is dropped down.
        // So the bar is asked for one that is.
        [[nodiscard]] Position itemPixelOn(WidgetId id) const
        {
            const auto centre = widgetCentre(
                toolbar.describe(
                    kCanvas,
                    Pointer{},
                    camera,
                    antwika::game::BuildTool::Road,
                    false,
                    0,
                    antwika::game::CityRatings{},
                    true),
                id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        void openGameMenu()
        {
            pressOn(widgets::kGameMenu);
        }

        void chooseItem(MenuItem item)
        {
            openGameMenu();
            pressAt(itemPixelOn(widgets::menuItemWidget(item)));
        }

        // The modal is laid out against the same canvas the bar is.
        [[nodiscard]] Position modalPixelOn(WidgetId id) const
        {
            const auto centre = widgetCentre(
                modalScene.describe(kCanvas, Pointer{}), id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        void pressAt(Position at)
        {
            send(PointerMoved{.position = at});
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = at});
        }

        // The bar's menu button is the one route in.
        // F10 is a fullscreen toggle now, and reaches no sink at all.
        void openModal()
        {
            pressOn(widgets::kMenu);
        }

        // Through the fold first, as bootstrap() registers it.
        // What the sink reads is what the fold was just given.
        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void pressOn(WidgetId id)
        {
            pressAt(pixelOn(id));
        }

        void tick()
        {
            dispatch(
                TickEvent{
                    .tick = 0,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        Camera camera{kHome};
        UiOverlay overlay{kCanvas};
        InputEventCodec codec;
        InputFold input{codec};
        Toolbar toolbar{kTranslator};
        PauseState pause;

        antwika::game::tests::FakeMenuCommands commands;
        RoadDrag drag;
        MenuModalScene modalScene{kTranslator};
        antwika::game::CityRatings ratings;
        UiSink sink{
            camera,
            overlay,
            input,
            toolbar,
            pause,
            commands,
            drag,
            modalScene,
            camera,
            ratings};
    };
} // namespace

TEST_F(UiSinkTest, Press_ZoomsInOnTheZoomInButton)
{
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomIn);

    EXPECT_EQ(before + 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Press_ZoomsOutOnTheZoomOutButton)
{
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomOut);

    EXPECT_EQ(before - 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Press_PutsTheCameraBackOnTheResetButton)
{
    const auto home = camera;

    camera.panBy(100, 100);
    camera.zoomOut();
    pressOn(widgets::kResetView);

    EXPECT_EQ(home, camera);
}

// The button reaches simulation state, inside the tick path.
// It defines no event of its own: what a replay holds is the click.
TEST_F(UiSinkTest, Press_HoldsTheSimulationStillOnThePauseButton)
{
    pressOn(widgets::kPauseResume);

    EXPECT_TRUE(pause.paused());
}

TEST_F(UiSinkTest, Press_LetsTheRunGoAgainOnTheSecondPress)
{
    pressOn(widgets::kPauseResume);
    pressOn(widgets::kPauseResume);

    EXPECT_FALSE(pause.paused());
}

// Otherwise the button would still say "pause" on the tick it paused on.
TEST_F(UiSinkTest, Press_RelabelsTheButtonInTheSameTickItWasPressed)
{
    pressOn(widgets::kPauseResume);

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"resume"}));
}

// The bar reports the tick the run is on, off the event being handled.
TEST_F(UiSinkTest, Tick_ReportsTheTickTheEventCarried)
{
    dispatch(
        TickEvent{
            .tick = 12,
            .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(
        toolbar
            .describe(
                kCanvas, Pointer{}, camera, overlay.tool(), false, 12)
            .commands,
        overlay.commands());
}

TEST_F(UiSinkTest, Press_LeavesTheCameraAloneAwayFromTheBar)
{
    const auto before = camera;

    send(PointerMoved{.position = kOnTheGrid});
    send(
        PointerButtonPressed{
            .button = MouseButton::Left, .position = kOnTheGrid});

    EXPECT_EQ(before, camera);
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(UiSinkTest, Press_ReportsTheBarCoveringWhatWasClicked)
{
    pressOn(widgets::kZoomIn);

    EXPECT_TRUE(overlay.pointerOverUi());
}

// A right-click is the grid's gesture, and never a button's.
TEST_F(UiSinkTest, RightPress_ActivatesNothing)
{
    const auto before = camera;
    const auto at = pixelOn(widgets::kZoomIn);

    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Right, .position = at});

    EXPECT_EQ(before, camera);
}

// Activation is on the press, so a release is not one.
// A release still says where the pointer is, hence no movement first.
TEST_F(UiSinkTest, Release_ActivatesNothing)
{
    const auto before = camera;

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOn(widgets::kZoomIn)});

    EXPECT_EQ(before, camera);
    EXPECT_TRUE(overlay.pointerOverUi());
}

// A press carries its own position.
// So it can be the first thing that ever says where the pointer is.
TEST_F(UiSinkTest, Press_NeedsNoMovementBeforeIt)
{
    const auto before = camera.zoomLevel();

    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = pixelOn(widgets::kZoomIn)});

    EXPECT_EQ(before + 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Tick_DrawsTheBarEvenBeforeAnythingIsClicked)
{
    tick();

    EXPECT_FALSE(overlay.commands().empty());
}

// The label has to show the zoom the tick ends at.
// Otherwise the picture would trail the state by a tick.
TEST_F(UiSinkTest, Tick_DescribesTheBarAgainAfterTheStateChanged)
{
    pressOn(widgets::kZoomIn);
    const auto afterPress = overlay.commands();

    tick();

    EXPECT_EQ(afterPress, overlay.commands());
}

// A key says nothing about where the pointer is.
// So one arriving first must not put it in the corner the bar is in.
TEST_F(UiSinkTest, KeyPress_LeavesThePointerNowhere)
{
    send(KeyPressed{.key = antwika::input::Key::A});

    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(UiSinkTest, Handle_IgnoresAnEventThatIsNotInput)
{
    const auto before = camera;

    dispatch(
        TickEvent{.tick = 0, .event = Event{.name = "game.started"}});

    EXPECT_EQ(before, camera);
    EXPECT_TRUE(overlay.commands().empty());
}

// The bar's menu button is the whole route in.
// It defines no event of its own.
// What a replay holds is the click, and works the widget out again.
TEST_F(UiSinkTest, Press_OpensTheModalOnTheMenuButton)
{
    openModal();

    EXPECT_TRUE(sink.menuOpen());
    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"Main Menu"}));
}

// Both calls openModal() makes say what they want rather than flip it.
TEST_F(UiSinkTest, OpeningTwice_LeavesTheModalOpen)
{
    openModal();
    openModal();

    EXPECT_TRUE(sink.menuOpen());
}

// The city is still there behind it, so the bar is still drawn.
TEST_F(UiSinkTest, ModalOpen_LeavesTheBarInThePicture)
{
    openModal();

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"zoom out"}));
}

// The scrim covers the canvas, which keeps a press off the city.
// GridSink already skips a press the overlay reports covered.
TEST_F(UiSinkTest, ModalOpen_ReportsThePointerCoveredOverTheGrid)
{
    openModal();
    pressAt(kOnTheGrid);

    EXPECT_TRUE(overlay.pointerOverUi());
}

// A press is resolved against the modal alone, never through it.
TEST_F(UiSinkTest, ModalOpen_LeavesTheBarUnpressable)
{
    openModal();
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomIn);

    EXPECT_EQ(before, camera.zoomLevel());
}

TEST_F(UiSinkTest, Modal_ClosesOnTheBackItem)
{
    openModal();

    pressAt(modalPixelOn(modalWidgets::kResume));

    EXPECT_FALSE(sink.menuOpen());
    EXPECT_EQ(0U, commands.mainMenus);
}

// Leaving is a mode change, staged like every other one.
TEST_F(UiSinkTest, Modal_AsksForTheMainMenuOnTheMainMenuItem)
{
    openModal();

    pressAt(modalPixelOn(modalWidgets::kMainMenu));

    EXPECT_EQ(1U, commands.mainMenus);
    EXPECT_FALSE(sink.menuOpen());
}

// There is no dismiss-by-backdrop rule here.
TEST_F(UiSinkTest, Modal_StaysUpOnAPressOnTheScrim)
{
    openModal();

    pressAt(kOnTheGrid);

    EXPECT_TRUE(sink.menuOpen());
}

// The modal holds nothing, and neither does entering a city.
// A run progresses until a player asks for a pause -- see PauseState.
TEST_F(UiSinkTest, Opening_LeavesTheRunGoing)
{
    openModal();

    EXPECT_FALSE(pause.paused());
}

// And a run somebody did pause is still paused behind the modal.
// Opening one says nothing either way about a pause.
TEST_F(UiSinkTest, Opening_LeavesAPausedRunPaused)
{
    pressOn(widgets::kPauseResume);

    openModal();

    EXPECT_TRUE(pause.paused());
}

// What a drag lays is what its release said.
// A release arriving over the modal never said it.
// So the gesture is over.
TEST_F(UiSinkTest, Opening_EndsARoadDragInProgress)
{
    drag.begin(Cell{.x = 2, .y = 3});

    openModal();

    EXPECT_FALSE(drag.active());
}

// The list is simulation state: it decides what a click means.
// It defines no event of its own, exactly as the modal does.
TEST_F(UiSinkTest, Press_DropsTheGameMenuDownOnItsBox)
{
    openGameMenu();

    EXPECT_TRUE(sink.gameMenuOpen());
    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"world map"}));
}

// Said as the opposite of what is showing rather than as a flip.
TEST_F(UiSinkTest, Press_PutsTheGameMenuAwayOnASecondPressOnItsBox)
{
    openGameMenu();

    pressOn(widgets::kGameMenu);

    EXPECT_FALSE(sink.gameMenuOpen());
}

// A closed list is a box and nothing else.
// So nothing behind it is listed, and no item can be pressed.
TEST_F(UiSinkTest, Closed_ListsNoItemAtAll)
{
    tick();

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Not(::testing::Contains(std::string{"world map"})));
}

TEST_F(UiSinkTest, Choose_StartsANewGameOnTheNewGameItem)
{
    chooseItem(MenuItem::NewGame);

    EXPECT_EQ(1U, commands.newGames);
}

// One screen, and both items lead to it -- see IMenuCommands.
TEST_F(UiSinkTest, Choose_OpensThePickerOnTheSaveGameItem)
{
    chooseItem(MenuItem::SaveGame);

    EXPECT_EQ(1U, commands.saves);
}

TEST_F(UiSinkTest, Choose_OpensThePickerOnTheLoadGameItem)
{
    chooseItem(MenuItem::LoadGame);

    EXPECT_EQ(1U, commands.saves);
}

TEST_F(UiSinkTest, Choose_LeavesForTheMainMenuOnTheMainMenuItem)
{
    chooseItem(MenuItem::MainMenu);

    EXPECT_EQ(1U, commands.mainMenus);
}

TEST_F(UiSinkTest, Choose_LeavesForTheWorldMapOnTheWorldMapItem)
{
    chooseItem(MenuItem::WorldMap);

    EXPECT_EQ(1U, commands.worldMaps);
}

// One item, one command.
// Two of them for one click is the trap Events.hpp describes.
TEST_F(UiSinkTest, Choose_RunsNothingElseTheListOffers)
{
    chooseItem(MenuItem::WorldMap);

    EXPECT_EQ(0U, commands.newGames);
    EXPECT_EQ(0U, commands.saves);
    EXPECT_EQ(0U, commands.mainMenus);
}

TEST_F(UiSinkTest, Choose_PutsTheListAway)
{
    chooseItem(MenuItem::NewGame);

    EXPECT_FALSE(sink.gameMenuOpen());
}

// Nothing but a press closes it.
// Otherwise every tick would put it away before it was read.
TEST_F(UiSinkTest, Tick_LeavesAnOpenListOpen)
{
    openGameMenu();

    tick();

    EXPECT_TRUE(sink.gameMenuOpen());
}

TEST_F(UiSinkTest, PressAwayFromAnOpenList_PutsItAway)
{
    openGameMenu();

    pressAt(kOnTheGrid);

    EXPECT_FALSE(sink.gameMenuOpen());
}

// And that press does nothing else at all.
// GridSink skips a press the overlay reports covered.
// So reporting it covered is what keeps that press off the city.
// One click dismissing a menu and laying a road is two things.
TEST_F(UiSinkTest, PressAwayFromAnOpenList_IsTheListsAndNotTheCitys)
{
    openGameMenu();

    pressAt(kOnTheGrid);

    EXPECT_TRUE(overlay.pointerOverUi());
}

TEST_F(UiSinkTest, PressOnTheBarWithAnOpenList_OnlyPutsItAway)
{
    openGameMenu();
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomIn);

    EXPECT_EQ(before, camera.zoomLevel());
    EXPECT_FALSE(sink.gameMenuOpen());
}

// The modal takes precedence over the bar entirely, the menu included.
TEST_F(UiSinkTest, ModalOpen_LeavesTheGameMenuUnpressable)
{
    openModal();

    pressOn(widgets::kGameMenu);

    EXPECT_FALSE(sink.gameMenuOpen());
}
