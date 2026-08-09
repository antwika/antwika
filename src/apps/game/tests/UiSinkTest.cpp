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


#include "FakeMenuCommands.hpp"
#include "Translators.hpp"
#include "WidgetCentre.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IMenuCommands.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/UiSink.hpp"
#include "antwika/game/ViewCommands.hpp"

using antwika::game::ViewCommands;
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

        [[nodiscard]] Position viewItemPixelOn(WidgetId id) const
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
                    false,
                    mapView.view(),
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

        void openModal()
        {
            pressOn(widgets::kMenu);
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
        antwika::game::MapViewState mapView;

        antwika::game::tests::FakeMenuCommands commands;
        RoadDrag drag;
        MenuModalScene modalScene{kTranslator};
        antwika::game::CityRatings ratings;
        antwika::game::GameState state;
        ViewCommands viewCommands{camera, pause, camera};
        UiSink sink{
            camera,
            overlay,
            input,
            toolbar,
            pause,
            mapView,
            commands,
            drag,
            modalScene,
            viewCommands,
            ratings,
            state};
    };
}

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

TEST_F(UiSinkTest, Press_RelabelsTheButtonInTheSameTickItWasPressed)
{
    pressOn(widgets::kPauseResume);

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"resume"}));
}

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

TEST_F(UiSinkTest, Tick_DescribesTheBarAgainAfterTheStateChanged)
{
    pressOn(widgets::kZoomIn);
    const auto afterPress = overlay.commands();

    tick();

    EXPECT_EQ(afterPress, overlay.commands());
}

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

TEST_F(UiSinkTest, Press_OpensTheModalOnTheMenuButton)
{
    openModal();

    EXPECT_TRUE(sink.menuOpen());
    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"Main Menu"}));
}

TEST_F(UiSinkTest, OpeningTwice_LeavesTheModalOpen)
{
    openModal();
    openModal();

    EXPECT_TRUE(sink.menuOpen());
}

TEST_F(UiSinkTest, ModalOpen_LeavesTheBarInThePicture)
{
    openModal();

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"zoom out"}));
}

TEST_F(UiSinkTest, ModalOpen_ReportsThePointerCoveredOverTheGrid)
{
    openModal();
    pressAt(kOnTheGrid);

    EXPECT_TRUE(overlay.pointerOverUi());
}

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

TEST_F(UiSinkTest, Modal_AsksForTheMainMenuOnTheMainMenuItem)
{
    openModal();

    pressAt(modalPixelOn(modalWidgets::kMainMenu));

    EXPECT_EQ(1U, commands.mainMenus);
    EXPECT_FALSE(sink.menuOpen());
}

TEST_F(UiSinkTest, Modal_StaysUpOnAPressOnTheScrim)
{
    openModal();

    pressAt(kOnTheGrid);

    EXPECT_TRUE(sink.menuOpen());
}

TEST_F(UiSinkTest, Opening_LeavesTheRunGoing)
{
    openModal();

    EXPECT_FALSE(pause.paused());
}

TEST_F(UiSinkTest, Opening_LeavesAPausedRunPaused)
{
    pressOn(widgets::kPauseResume);

    openModal();

    EXPECT_TRUE(pause.paused());
}

TEST_F(UiSinkTest, Opening_EndsARoadDragInProgress)
{
    drag.begin(Cell{.x = 2, .y = 3});

    openModal();

    EXPECT_FALSE(drag.active());
}

TEST_F(UiSinkTest, Press_DropsTheGameMenuDownOnItsBox)
{
    openGameMenu();

    EXPECT_TRUE(sink.gameMenuOpen());
    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"world map"}));
}

TEST_F(UiSinkTest, Press_PutsTheGameMenuAwayOnASecondPressOnItsBox)
{
    openGameMenu();

    pressOn(widgets::kGameMenu);

    EXPECT_FALSE(sink.gameMenuOpen());
}

TEST_F(UiSinkTest, Press_DropsTheOverlayMenuDownOnItsBox)
{
    pressOn(widgets::kViewMenu);

    EXPECT_TRUE(sink.viewMenuOpen());
    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"desirability"}));
}

TEST_F(UiSinkTest, Press_ChoosesTheViewTheListWasPressedOn)
{
    pressOn(widgets::kViewMenu);
    pressAt(
        viewItemPixelOn(
            widgets::viewWidget(antwika::game::MapView::Desirability)));

    EXPECT_EQ(mapView.view(), antwika::game::MapView::Desirability);
    EXPECT_FALSE(sink.viewMenuOpen());
}

TEST_F(UiSinkTest, Closed_NamesTheViewThatIsShowing)
{
    mapView.set(antwika::game::MapView::Fire);
    tick();

    EXPECT_THAT(
        textsOf(overlay.commands()),
        ::testing::Contains(std::string{"fire"}));
}

TEST_F(UiSinkTest, Press_PutsTheOtherListAwayWhenOneIsOpened)
{
    openGameMenu();
    ASSERT_TRUE(sink.gameMenuOpen());

    pressOn(widgets::kViewMenu);

    EXPECT_FALSE(sink.gameMenuOpen());
    EXPECT_FALSE(sink.viewMenuOpen());

    pressOn(widgets::kViewMenu);

    EXPECT_TRUE(sink.viewMenuOpen());
    EXPECT_FALSE(sink.gameMenuOpen());
}

TEST_F(UiSinkTest, Press_PutsTheOverlayMenuAwayOnASecondPressOnItsBox)
{
    pressOn(widgets::kViewMenu);
    pressOn(widgets::kViewMenu);

    EXPECT_FALSE(sink.viewMenuOpen());
    EXPECT_EQ(mapView.view(), antwika::game::MapView::Normal);
}

TEST_F(UiSinkTest, Press_PutsTheOverlayMenuAwayOnAPressOffIt)
{
    pressOn(widgets::kViewMenu);
    pressOn(widgets::kZoomIn);

    EXPECT_FALSE(sink.viewMenuOpen());
    EXPECT_EQ(mapView.view(), antwika::game::MapView::Normal);
}

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

TEST_F(UiSinkTest, ModalOpen_LeavesTheGameMenuUnpressable)
{
    openModal();

    pressOn(widgets::kGameMenu);

    EXPECT_FALSE(sink.gameMenuOpen());
}
