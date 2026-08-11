#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <antwika/console/ConsoleState.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorConsole.hpp"

using antwika::console::ConsoleState;
using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::EditorCommands;
using antwika::map_editor::EditorConsoleSystem;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::pinAll;
using antwika::map_editor::TilesetDoc;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Rgb;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::TileMap;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kTick{};

    [[nodiscard]] EditorStore storeOf()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        pinAll(store.state);
        store.windowSize = antwika::gfx::Size{
            .width = 960, .height = 810};

        return store;
    }

    /**
     * @brief Ticks until the console finishes animating.
     */
    void settle(
        EditorConsoleSystem &system, World &world, EditorStore &store)
    {
        for (int frame = 0; frame < 16; ++frame)
        {
            store.input.events.clear();
            system.update(world, kTick);
        }
    }

    [[nodiscard]] std::string lastLine(const ConsoleState &console)
    {
        return console.history().empty() ? std::string{}
                                         : console.history().back();
    }

    class EditorConsoleTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        EditorStore store = storeOf();
        ConsoleState console;

        void run(const std::string &line)
        {
            EditorCommands commands{store, logger};
            commands.execute(line, console);
        }
    };
}

TEST_F(EditorConsoleTest, Names_ListsEveryCommandItAnswers)
{
    EditorCommands commands{store, logger};

    EXPECT_EQ(
        commands.names(),
        (std::vector<std::string>{
            "help",
            "open",
            "save",
            "generate",
            "validate",
            "scale",
            "palette"}));
}

TEST_F(EditorConsoleTest, Execute_PrintsEveryHelpLine)
{
    run("help");

    EXPECT_EQ(console.history().size(), 8U);
    EXPECT_EQ(lastLine(console), "quit - close the editor");
}

TEST_F(EditorConsoleTest, Execute_ReportsAnUnknownCommand)
{
    run("fly");

    EXPECT_EQ(lastLine(console), "unknown command: fly - try help");
}

TEST_F(EditorConsoleTest, Open_AsksForAFileWhenGivenNoArgument)
{
    run("open");

    EXPECT_EQ(lastLine(console), "open: name a file to load");
}

TEST_F(EditorConsoleTest, Open_ReportsAFileItCannotLoad)
{
    const ScratchDirectory scratch("console.");

    run("open " + (scratch.path() / "absent.json").string());

    EXPECT_THAT(lastLine(console), ::testing::StartsWith("open: "));
}

TEST_F(EditorConsoleTest, Open_LoadsTheMapAndClearsTheSelection)
{
    const ScratchDirectory scratch("console.");
    const auto where = scratch.path() / "other.json";
    saveMapFile(where, TileMap{MapHeader{.id = "other"}, 2, 2});
    store.ui.selected = 3;
    store.ui.idField.text = "stale";

    run("open " + where.string());

    EXPECT_EQ(store.state.map.header().id, "other");
    EXPECT_FALSE(store.ui.selected.has_value());
    EXPECT_TRUE(store.ui.idField.text.empty());
    EXPECT_EQ(lastLine(console), "opened " + where.string());
}

TEST_F(EditorConsoleTest, Save_WritesToTheCurrentPathWithNoArgument)
{
    const ScratchDirectory scratch("console.");
    store.state.path = scratch.path() / "map.json";

    run("save");

    EXPECT_TRUE(std::filesystem::exists(store.state.path));
    EXPECT_EQ(
        lastLine(console), "saved " + store.state.path.string());
}

TEST_F(EditorConsoleTest, Save_WritesToTheNamedPath)
{
    const ScratchDirectory scratch("console.");
    const auto where = scratch.path() / "named.json";

    run("save " + where.string());

    EXPECT_TRUE(std::filesystem::exists(where));
    EXPECT_EQ(store.state.path, where);
}

TEST_F(EditorConsoleTest, Save_ReportsAPathItCannotWrite)
{
    const ScratchDirectory scratch("console.");

    run("save " + (scratch.path() / "absent" / "m.json").string());

    EXPECT_THAT(lastLine(console), ::testing::StartsWith("save: "));
}

TEST_F(EditorConsoleTest, Save_ReportsTheTilesetMessageInTheTilesView)
{
    store.view = EditorView::Tiles;

    run("save");

    EXPECT_THAT(lastLine(console), ::testing::StartsWith("save: "));
}

TEST_F(EditorConsoleTest, Save_WritesTheActiveTilesetInTheTilesView)
{
    const ScratchDirectory scratch("console.");
    store.view = EditorView::Tiles;
    TilesetDoc doc;
    doc.data.name = "rustwall";
    doc.path = scratch.path() / "rustwall";
    store.tilesets.open.push_back(std::move(doc));

    run("save");

    EXPECT_EQ(lastLine(console), "saved tileset rustwall");
}

TEST_F(EditorConsoleTest, Generate_RunsWithTheStandingSeed)
{
    store.state.pinned.assign(store.state.pinned.size(), false);

    run("generate");

    EXPECT_THAT(lastLine(console), ::testing::StartsWith("generated "));
}

TEST_F(EditorConsoleTest, Generate_TakesASeedArgument)
{
    store.state.pinned.assign(store.state.pinned.size(), false);

    run("generate 42");

    EXPECT_EQ(lastLine(console), "generated (seed 42)");
}

TEST_F(EditorConsoleTest, Generate_RefusesASeedThatIsNotANumber)
{
    run("generate soon");

    EXPECT_EQ(lastLine(console), "generate: the seed is a number");
}

TEST_F(EditorConsoleTest, Generate_ReportsAFailedRun)
{
    store.state.pinned.assign(store.state.pinned.size(), false);

    for (auto &row : store.state.rules.allowed)
    {
        row.fill(false);
    }

    run("generate 7");

    EXPECT_EQ(lastLine(console), "generate failed (seed 7)");
}

TEST_F(EditorConsoleTest, Validate_CountsTheFindings)
{
    run("validate");

    EXPECT_TRUE(store.state.report.has_value());
    EXPECT_THAT(lastLine(console), ::testing::StartsWith("validated: "));
}

TEST_F(EditorConsoleTest, Scale_TakesAScaleInsideTheRange)
{
    run("scale 4");

    EXPECT_EQ(store.pendingUiScale, 4U);
    EXPECT_EQ(lastLine(console), "scale set to 4x");
}

TEST_F(EditorConsoleTest, Scale_RefusesAScaleOutsideTheRange)
{
    run("scale 1");
    EXPECT_EQ(lastLine(console), "scale: say scale <2|3|4>");

    run("scale 5");
    EXPECT_EQ(lastLine(console), "scale: say scale <2|3|4>");

    run("scale wide");
    EXPECT_EQ(lastLine(console), "scale: say scale <2|3|4>");

    EXPECT_FALSE(store.pendingUiScale.has_value());
}

TEST_F(EditorConsoleTest, Palette_SetsTheInkAndLeavesThePaper)
{
    const auto paper = store.state.map.header().paper;

    run("palette ink #10a0ff");

    EXPECT_EQ(
        store.state.map.header().ink,
        (Rgb{.red = 0x10, .green = 0xA0, .blue = 0xFF}));
    EXPECT_EQ(store.state.map.header().paper, paper);
    EXPECT_EQ(lastLine(console), "palette ink set to #10a0ff");
}

TEST_F(EditorConsoleTest, Palette_SetsThePaperAndLeavesTheInk)
{
    const auto ink = store.state.map.header().ink;

    run("palette paper 0b0c0d");

    EXPECT_EQ(
        store.state.map.header().paper,
        (Rgb{.red = 0x0B, .green = 0x0C, .blue = 0x0D}));
    EXPECT_EQ(store.state.map.header().ink, ink);
}

TEST_F(EditorConsoleTest, Palette_RefusesAnUnknownChannelOrColor)
{
    run("palette border #ff0000");
    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");

    run("palette ink nope");
    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");

    run("palette");
    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(EditorConsoleTest, Execute_TrimsARunOfSpacesToNoArgument)
{
    run("open    ");

    EXPECT_EQ(lastLine(console), "open: name a file to load");
}

TEST(EditorConsoleSystemTest, Update_ReportsTheConsoleAsHidden)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    system.update(world, kTick);

    EXPECT_FALSE(store.input.consoleVisible);
    EXPECT_EQ(store.input.consoleHeightCanvas, 0);
}

TEST(EditorConsoleSystemTest, Update_RebuildsTheOverlayForANewWindow)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    system.update(world, kTick);
    store.windowSize = antwika::gfx::Size{
        .width = 640, .height = 480};
    system.update(world, kTick);

    EXPECT_EQ(system.picture().canvas(), store.windowSize);
}

TEST(EditorConsoleSystemTest, Update_FoldsAKeyPressThroughTheConsole)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    EditorConsoleSystem system{store, logger};

    system.update(world, kTick);
    settle(system, world, store);

    EXPECT_TRUE(store.input.consoleVisible);
    EXPECT_GT(store.input.consoleHeightCanvas, 0);
}

TEST(EditorConsoleSystemTest, Update_ClosesAVisibleConsoleOnEscape)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    system.update(world, kTick);
    settle(system, world, store);
    ASSERT_TRUE(store.input.consoleVisible);

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Escape});
    system.update(world, kTick);
    settle(system, world, store);

    EXPECT_FALSE(store.input.consoleVisible);
}

TEST(EditorConsoleSystemTest, Update_IgnoresARepeatedEscape)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    system.update(world, kTick);
    settle(system, world, store);

    store.input.events.push_back(antwika::input::KeyPressed{
        .key = antwika::input::Key::Escape, .repeat = true});
    system.update(world, kTick);
    settle(system, world, store);

    EXPECT_TRUE(store.input.consoleVisible);
}

TEST(EditorConsoleSystemTest, Update_IgnoresEscapeWhileTheConsoleIsShut)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Escape});
    EditorConsoleSystem system{store, logger};

    system.update(world, kTick);

    EXPECT_FALSE(store.input.consoleVisible);
}

TEST_F(EditorConsoleTest, Scale_RefusesANumberWithTrailingText)
{
    run("scale 2x");

    EXPECT_EQ(lastLine(console), "scale: say scale <2|3|4>");
}

TEST(EditorConsoleSystemTest, Update_KeepsTheConsoleOpenOnAnotherKey)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    system.update(world, kTick);
    settle(system, world, store);

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::A});
    system.update(world, kTick);
    settle(system, world, store);

    EXPECT_TRUE(store.input.consoleVisible);
}

TEST(EditorConsoleSystemTest, Update_QuitsWhenTheConsoleAsksToStop)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    system.update(world, kTick);
    settle(system, world, store);

    for (const auto key : {
             antwika::input::Key::Q,
             antwika::input::Key::U,
             antwika::input::Key::I,
             antwika::input::Key::T,
             antwika::input::Key::Enter})
    {
        store.input.events.clear();
        store.input.events.push_back(
            antwika::input::KeyPressed{.key = key});
        system.update(world, kTick);
    }

    EXPECT_TRUE(store.input.quit);
}

TEST(EditorConsoleSystemTest, Update_TakesOnlyANonRepeatedEscape)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyPressed{.key = antwika::input::Key::Grave});
    system.update(world, kTick);
    settle(system, world, store);
    ASSERT_TRUE(store.input.consoleVisible);

    store.input.events.push_back(antwika::input::KeyPressed{
        .key = antwika::input::Key::Escape, .repeat = true});
    store.input.events.push_back(antwika::input::KeyPressed{
        .key = antwika::input::Key::Escape, .repeat = false});
    system.update(world, kTick);
    settle(system, world, store);

    EXPECT_FALSE(store.input.consoleVisible);
}

TEST(EditorConsoleSystemTest, Update_PassesOverAnEventThatIsNotAKeyPress)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EditorConsoleSystem system{store, logger};

    store.input.events.push_back(
        antwika::input::KeyReleased{.key = antwika::input::Key::A});
    system.update(world, kTick);

    EXPECT_FALSE(store.input.consoleVisible);
}
