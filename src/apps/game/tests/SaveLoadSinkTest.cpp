#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#include <antwika/ecs/World.hpp>
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
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetPixel.hpp"

#include "TestTranslator.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveDirectory.hpp"
#include "antwika/game/SaveGameFile.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadSink.hpp"
#include "antwika/game/SaveLoadState.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::game::tests::kTranslator;

namespace
{

        using antwika::game::tests::widgetCentre;
    using antwika::ecs::World;
    using antwika::event::Event;
    using antwika::event::TickEvent;
    using antwika::game::AppMode;
    using antwika::game::AppModeState;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::GameState;
    using antwika::game::GridExtent;
    using antwika::game::InputFold;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::SaveGame;
    using antwika::game::SaveLoadScene;
    using antwika::game::SaveLoadSink;
    using antwika::game::SaveLoadState;
    using antwika::game::SessionStore;
    using antwika::game::UiOverlay;
    using antwika::gfx::Point;
    using antwika::gfx::Size;
    using antwika::input::InputEvent;
    using antwika::input::InputEventCodec;
    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerMoved;
    using antwika::input::Position;
    using antwika::log::mocks::MockLogger;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;
    namespace saveWidgets = antwika::game::saveWidgets;

    constexpr Size kCanvas{.width = 1024, .height = 640};
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    class SaveLoadSinkTest : public ::testing::Test
    {
    protected:
        SaveLoadSinkTest()
            : directory(
                  antwika::testing::scratchPath("antwika_saveload_"))
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
            std::filesystem::create_directories(directory, ignored);
        }

        ~SaveLoadSinkTest() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        SaveLoadSinkTest(const SaveLoadSinkTest &) = delete;
        SaveLoadSinkTest(SaveLoadSinkTest &&) = delete;

        SaveLoadSinkTest &operator=(const SaveLoadSinkTest &) = delete;
        SaveLoadSinkTest &operator=(SaveLoadSinkTest &&) = delete;

        // Where a widget is, is the layout's business.
        // So a test asks the layout for the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            const auto centre = widgetCentre(
                scene.describe(kCanvas, Pointer{}, Keyboard{}, state), id);

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

        void tick()
        {
            dispatch(
                TickEvent{
                    .tick = 0,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        void pressAt(Position at)
        {
            send(PointerMoved{.position = at});
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = at});
        }

        void press(WidgetId id)
        {
            pressAt(pixelOn(id));
        }

        void type(Key key, bool shift = false)
        {
            send(
                KeyPressed{.key = key, .modifiers = {.shift = shift}});
        }

        void writeSave(const std::string &name, Cell at)
        {
            SaveGame save;
            save.paths = {at};
            save.extent = kExtent;
            antwika::game::saveGameFile(
                save,
                antwika::game::saveGamePath(directory.string(), name));
        }

        [[nodiscard]] std::string pathFor(const std::string &name) const
        {
            return antwika::game::saveGamePath(directory.string(), name);
        }

        void layPath(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
            paths.insert(cell);
            world.commit();
        }

        std::filesystem::path directory;
        InputEventCodec codec;
        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        antwika::game::BuildingIndex built;
        Camera camera;
        GameState gameState;
        SessionStore session{
            world, paths, built, camera, gameState, kExtent, 5};
        AppModeState mode{AppMode::SaveLoad};
        UiOverlay overlay{kCanvas};
        InputFold input{codec};
        SaveLoadScene scene{kTranslator};
        SaveLoadState state;
        antwika::game::OptionsState options;
        SaveLoadSink sink{
            state,
            mode,
            overlay,
            input,
            scene,
            session,
            options,
            directory.string()};
    };

    TEST_F(SaveLoadSinkTest, TheScreenIsDescribedForTheRendererEveryTick)
    {
        EXPECT_TRUE(overlay.commands().empty());

        tick();

        EXPECT_FALSE(overlay.commands().empty());
    }

    TEST_F(SaveLoadSinkTest, NothingHappensOutsideTheSaveLoadMode)
    {
        mode.request(AppMode::MainMenu);
        mode.handle(TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}});

        tick();
        press(saveWidgets::kBack);

        EXPECT_TRUE(overlay.commands().empty());
    }

    TEST_F(SaveLoadSinkTest, AnEventThatIsNotInputIsIgnored)
    {
        dispatch(
            TickEvent{
                .tick = 0,
                .event = Event{.name = "game.score_increment"}});

        EXPECT_TRUE(overlay.commands().empty());
    }

    TEST_F(SaveLoadSinkTest, BackGoesToTheMainMenu)
    {
        press(saveWidgets::kBack);

        EXPECT_EQ(mode.next(), AppMode::MainMenu);
    }

    TEST_F(SaveLoadSinkTest, TypingIntoTheFieldIsKeptByTheApplication)
    {
        press(saveWidgets::kName);
        type(Key::A);
        type(Key::B, true);

        EXPECT_EQ(state.name(), "aB");
        EXPECT_EQ(state.caret(), 2U);
    }

    TEST_F(SaveLoadSinkTest, TabReachesTheFieldWithoutThePointer)
    {
        // The picker first, then the field.
        type(Key::Tab);
        type(Key::Tab);
        type(Key::T);

        EXPECT_EQ(state.name(), "t");
    }

    TEST_F(SaveLoadSinkTest, BackspaceTakesTheCharacterBeforeTheCaret)
    {
        press(saveWidgets::kName);
        type(Key::A);
        type(Key::B);
        type(Key::Backspace);

        EXPECT_EQ(state.name(), "a");
    }

    TEST_F(SaveLoadSinkTest, SavingWritesTheSessionUnderTheTypedName)
    {
        layPath(Cell{.x = 2, .y = 3});

        press(saveWidgets::kName);
        type(Key::T);
        press(saveWidgets::kSave);

        EXPECT_EQ(state.message(), "Saved t");
        ASSERT_TRUE(std::filesystem::exists(pathFor("t")));
        EXPECT_EQ(
            antwika::game::loadGameFile(pathFor("t")).paths,
            (std::vector<Cell>{{.x = 2, .y = 3}}));

        // The name is now in the list, and is what is selected.
        // Re-listing the directory here would not replay.
        ASSERT_EQ(state.options().size(), 1U);
        EXPECT_EQ(state.selectedName(), "t");
        EXPECT_TRUE(state.name().empty());
    }

    TEST_F(SaveLoadSinkTest, EnterInTheFieldSavesToo)
    {
        press(saveWidgets::kName);
        type(Key::T);
        type(Key::Enter);

        EXPECT_EQ(state.message(), "Saved t");
        EXPECT_TRUE(std::filesystem::exists(pathFor("t")));
    }

    TEST_F(SaveLoadSinkTest, SavingWithNoNameAtAllSaysSo)
    {
        press(saveWidgets::kSave);

        EXPECT_EQ(state.message(), "Name it first");
    }

    // With nothing typed, Save overwrites whatever is selected.
    TEST_F(SaveLoadSinkTest, SavingWithNothingTypedWritesTheSelectedOne)
    {
        writeSave("town", Cell{.x = 1, .y = 1});
        SaveLoadState listed({"town"});
        SaveLoadSink over{
            listed,
            mode,
            overlay,
            input,
            scene,
            session,
            options,
            directory.string()};

        layPath(Cell{.x = 5, .y = 5});

        const auto at = pixelOn(saveWidgets::kSave);
        input.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(PointerMoved{.position = at})});
        over.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(PointerMoved{.position = at})});
        input.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left, .position = at})});
        over.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left, .position = at})});

        EXPECT_EQ(listed.message(), "Saved town");
        EXPECT_EQ(
            antwika::game::loadGameFile(pathFor("town")).paths,
            (std::vector<Cell>{{.x = 5, .y = 5}}));
    }

    TEST_F(SaveLoadSinkTest, ASaveThatCannotBeWrittenSaysSo)
    {
        SaveLoadSink nowhere{
            state,
            mode,
            overlay,
            input,
            scene,
            session,
            options,
            (directory / "not-a-directory").string()};

        const auto at = pixelOn(saveWidgets::kName);
        input.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left, .position = at})});
        nowhere.handle(
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left, .position = at})});

        const TickEvent typed{
            .tick = 0, .event = codec.encode(KeyPressed{.key = Key::T})};
        input.handle(typed);
        nowhere.handle(typed);

        const TickEvent enter{
            .tick = 0,
            .event = codec.encode(KeyPressed{.key = Key::Enter})};
        input.handle(enter);
        nowhere.handle(enter);

        EXPECT_TRUE(state.message().starts_with("Could not save: "));
    }

    TEST_F(SaveLoadSinkTest, LoadingWithNothingSelectedSaysSo)
    {
        press(saveWidgets::kLoad);

        EXPECT_EQ(state.message(), "Pick a save first");
        EXPECT_EQ(mode.next(), AppMode::SaveLoad);
    }

    TEST_F(SaveLoadSinkTest, LoadingRestoresTheSessionAndOpensTheGrid)
    {
        writeSave("town", Cell{.x = 4, .y = 6});

        layPath(Cell{.x = 2, .y = 3});
        press(saveWidgets::kName);
        type(Key::T);
        press(saveWidgets::kSave);

        // Point the picker at the file written outside this session.
        state.add("town");
        press(saveWidgets::kLoad);
        world.commit();

        EXPECT_EQ(state.message(), "Loaded town");
        EXPECT_EQ(mode.next(), AppMode::CityMap);
        EXPECT_TRUE(paths.has(Cell{.x = 4, .y = 6}));
        EXPECT_FALSE(paths.has(Cell{.x = 2, .y = 3}));
    }

    TEST_F(SaveLoadSinkTest, ALoadThatCannotBeReadSaysSo)
    {
        state.add("missing");

        press(saveWidgets::kLoad);

        EXPECT_TRUE(state.message().starts_with("Could not load: "));
        EXPECT_EQ(mode.next(), AppMode::SaveLoad);
    }

    TEST_F(SaveLoadSinkTest, ThePickerOpensAndClosesOnItsBox)
    {
        writeSave("town", Cell{.x = 1, .y = 1});
        state.add("town");

        press(saveWidgets::kPicker);
        EXPECT_TRUE(state.listOpen());

        press(saveWidgets::kPicker);
        EXPECT_FALSE(state.listOpen());
    }

    TEST_F(SaveLoadSinkTest, PressingAnOptionSelectsItAndShutsTheList)
    {
        state.add("alpha");
        state.add("beta");
        state.setListOpen(true);

        press(
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(saveWidgets::kFirstOption) + 1));

        EXPECT_EQ(state.selectedName(), "beta");
        EXPECT_FALSE(state.listOpen());
    }

    // antwika::ui retains nothing between frames.
    // So the picture drawn is the one described *after* acting.
    TEST_F(SaveLoadSinkTest, ThePictureShowsWhatThePressJustChanged)
    {
        state.add("town");

        press(saveWidgets::kPicker);

        const auto opened = overlay.commands().size();

        press(saveWidgets::kPicker);

        EXPECT_GT(opened, overlay.commands().size());
    }

    // A press is the left button's, and only the left button's.
    TEST_F(SaveLoadSinkTest, AMiddlePressActivatesNothing)
    {
        const auto at = pixelOn(saveWidgets::kBack);

        send(PointerMoved{.position = at});
        send(
            PointerButtonPressed{
                .button = MouseButton::Middle, .position = at});

        EXPECT_EQ(mode.next(), AppMode::SaveLoad);
    }

} // namespace
