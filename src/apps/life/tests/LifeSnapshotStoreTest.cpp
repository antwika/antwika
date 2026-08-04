#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/LifeSnapshotStore.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::input::InputEventCodec;
using antwika::input::Position;
using antwika::life::Board;
using antwika::life::Cell;
using antwika::life::CellCoordinate;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::LifeSnapshotStore;
using antwika::life::PointerToggleSink;
using antwika::life::StateDump;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::gfx::Size kCanvas{.width = 20, .height = 20};

    [[nodiscard]] const antwika::console::SnapshotFormat &dumpFormat()
    {
        static const antwika::console::SnapshotFormat format(
            {.magic = antwika::life::kStateDumpMagic,
             .version = antwika::life::kStateDumpVersion},
            "antwika life state dump document",
            antwika::life::standardStateDumpMigrations);
        return format;
    }

    // The board, the drag and the sink one store stands over.
    struct StoreHarness
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        Grid grid{world, 2, 2};
        DragState drag;
        InputEventCodec codec;
        PointerToggleSink sink{world, grid, codec, kCanvas, drag};
        LifeSnapshotStore store{world, grid, drag, sink};

        StoreHarness()
        {
            world.commit();
        }
    };
} // namespace

TEST(LifeSnapshotStoreTest, DumpCarriesTheBoardTheDragAndTheConsole)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_dump.json");
    const auto path = file.path().string();

    StoreHarness harness;
    harness.world.set<Cell>(
        harness.grid.entityAt(1, 0), Cell{.alive = true});
    harness.world.commit();

    harness.drag.begin();
    harness.sink.restoreDrag(
        {harness.grid.entityAt(1, 0)}, Position{.x = 5, .y = 6});

    harness.store.dump(path, {"> dump_state"});

    const auto snapshot = dumpFormat().read(path);
    const auto state =
        antwika::life::stateDumpFromJson(snapshot.state);

    EXPECT_EQ(
        snapshot.console, std::vector<std::string>{"> dump_state"});
    EXPECT_EQ(
        state.board,
        (Board{
            .width = 2,
            .height = 2,
            .alive = {false, true, false, false}}));
    EXPECT_TRUE(state.dragging);
    EXPECT_EQ(
        state.visited,
        (std::vector<CellCoordinate>{{.x = 1, .y = 0}}));
    EXPECT_EQ(state.lastDrag, (Position{.x = 5, .y = 6}));
}

TEST(LifeSnapshotStoreTest, LoadComesBackToTheDumpedInstant)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_load.json");
    const auto path = file.path().string();

    StateDump dump;
    dump.board = Board{
        .width = 2, .height = 2, .alive = {true, false, false, true}};
    dump.dragging = true;
    dump.visited = {CellCoordinate{.x = 0, .y = 1}};
    dump.lastDrag = Position{.x = 3, .y = 9};

    dumpFormat().write(
        antwika::console::Snapshot{
            .console = {"> dump_state", "dumped state to " + path},
            .state = antwika::life::stateDumpToJson(dump)},
        path);

    StoreHarness harness;

    const auto console = harness.store.load(path);

    // Staged rather than switched, so the commit is what lands it.
    harness.world.commit();

    EXPECT_EQ(
        readBoard(harness.world, harness.grid), dump.board);
    EXPECT_TRUE(harness.drag.inProgress());
    EXPECT_EQ(
        harness.sink.visitedCells(),
        std::set<Entity>{harness.grid.entityAt(0, 1)});
    EXPECT_EQ(
        harness.sink.lastDragPosition(), (Position{.x = 3, .y = 9}));
    EXPECT_EQ(
        console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
}

TEST(LifeSnapshotStoreTest, LoadEndsADragTheDumpDidNotHold)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_load_still.json");
    const auto path = file.path().string();

    StateDump dump;
    dump.board = Board{
        .width = 2, .height = 2, .alive = {false, false, false, false}};

    dumpFormat().write(
        antwika::console::Snapshot{
            .console = {},
            .state = antwika::life::stateDumpToJson(dump)},
        path);

    StoreHarness harness;
    harness.drag.begin();

    const auto console = harness.store.load(path);

    EXPECT_FALSE(harness.drag.inProgress());
    EXPECT_TRUE(harness.sink.visitedCells().empty());
    EXPECT_EQ(harness.sink.lastDragPosition(), std::nullopt);
    EXPECT_TRUE(console.empty());
}

TEST(LifeSnapshotStoreTest, LoadRefusesABoardOfAnotherSize)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_load_size.json");
    const auto path = file.path().string();

    StateDump dump;
    dump.board = Board{
        .width = 3,
        .height = 2,
        .alive = {false, false, false, false, false, false}};

    dumpFormat().write(
        antwika::console::Snapshot{
            .console = {},
            .state = antwika::life::stateDumpToJson(dump)},
        path);

    StoreHarness harness;

    EXPECT_THROW(
        (void)harness.store.load(path),
        antwika::console::SnapshotError);
}

TEST(LifeSnapshotStoreTest, LoadRefusesAStateThisBuildCannotRead)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_load_bad.json");
    const auto path = file.path().string();

    StateDump dump;
    dump.board = Board{
        .width = 2, .height = 2, .alive = {false, false, false, false}};

    auto state = antwika::life::stateDumpToJson(dump);
    state["board"]["cells"] = "000";

    dumpFormat().write(
        antwika::console::Snapshot{.console = {}, .state = state},
        path);

    StoreHarness harness;

    EXPECT_THROW(
        (void)harness.store.load(path),
        antwika::console::SnapshotError);
}

TEST(LifeSnapshotStoreTest, AStoreWithoutAPointerSinkCarriesNoDrag)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_no_sink.json");
    const auto path = file.path().string();

    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();
    DragState drag;
    LifeSnapshotStore store(world, grid, drag, std::nullopt);

    store.dump(path, {});

    const auto snapshot = dumpFormat().read(path);
    const auto state =
        antwika::life::stateDumpFromJson(snapshot.state);

    EXPECT_TRUE(state.visited.empty());
    EXPECT_EQ(state.lastDrag, std::nullopt);

    // A load applies the board and the drag flag all the same.
    StateDump loaded;
    loaded.board = Board{
        .width = 2, .height = 2, .alive = {true, true, true, true}};
    loaded.dragging = true;

    dumpFormat().write(
        antwika::console::Snapshot{
            .console = {},
            .state = antwika::life::stateDumpToJson(loaded)},
        path);

    const auto console = store.load(path);
    world.commit();

    EXPECT_EQ(readBoard(world, grid), loaded.board);
    EXPECT_TRUE(drag.inProgress());
    EXPECT_TRUE(console.empty());
}

TEST(LifeSnapshotStoreTest, LoadRefusesABoardOfAnotherHeight)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_store_load_height.json");
    const auto path = file.path().string();

    // The width matches the running grid; only the height does not.
    StateDump dump;
    dump.board = Board{
        .width = 2,
        .height = 3,
        .alive = {false, false, false, false, false, false}};

    dumpFormat().write(
        antwika::console::Snapshot{
            .console = {},
            .state = antwika::life::stateDumpToJson(dump)},
        path);

    StoreHarness harness;

    EXPECT_THROW(
        (void)harness.store.load(path),
        antwika::console::SnapshotError);
}
