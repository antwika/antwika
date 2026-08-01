#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace
{

    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::CityGrid;
    using antwika::game::cityGridOf;
    using antwika::game::Direction;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::restoreCityGrid;
    using antwika::game::StoredBuilding;
    using antwika::game::StoredWalker;
    using antwika::game::Walker;
    using antwika::log::mocks::MockLogger;

    class CityGridTest : public ::testing::Test
    {
    protected:
        Entity layPath(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
            paths.insert(cell);
            return entity;
        }

        Entity putUp(Cell cell, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Building>(entity, Building{.kind = kind});
            return entity;
        }

        Entity sendOut(Cell cell, Entity home)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Walker>(entity, Walker{.home = home});
            return entity;
        }

        [[nodiscard]] std::size_t countOf() const
        {
            return world.view<Cell>().size();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
    };

    // Varying one member at a time, from a value that differs in none.
    // The idiom ValueEqualityTest uses, and for its reason.
    // A defaulted operator== is one comparison per member.
    // A test that varies one member proves only that member is in it.
    template <typename T, typename Mutate>
    void expectMemberCompared(const T &base, Mutate mutate)
    {
        T changed = base;
        mutate(changed);

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, base);
    }

    TEST_F(CityGridTest, CityGridOf_TakesEveryWalkerAndBuilding)
    {
        layPath(Cell{1, 1});
        putUp(Cell{2, 2}, BuildingKind::House);
        sendOut(Cell{1, 1}, kNullEntity);
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 1U);
        ASSERT_EQ(grid.walkers.size(), 1U);
        EXPECT_EQ(grid.buildings[0].at, (Cell{2, 2}));
        EXPECT_EQ(grid.walkers[0].at, (Cell{1, 1}));
    }

    TEST_F(CityGridTest, CityGridOf_TakesNothingFromAnEmptyGrid)
    {
        const auto grid = cityGridOf(world);

        EXPECT_EQ(grid, CityGrid{});
    }

    TEST_F(CityGridTest, CityGridOf_KeepsTheLinkAsAPairOfIndices)
    {
        const auto home = putUp(Cell{4, 4}, BuildingKind::Farm);
        const auto walker = sendOut(Cell{5, 4}, home);
        world.commit();

        auto sent = world.get<Building>(home);
        sent.walkers[0] = walker;
        world.set<Building>(home, sent);
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 1U);
        ASSERT_EQ(grid.walkers.size(), 1U);
        EXPECT_EQ(
            grid.buildings[0].walkers[0], std::optional<std::size_t>{0});
        EXPECT_FALSE(grid.buildings[0].walkers[1].has_value());
        EXPECT_EQ(grid.walkers[0].home, std::optional<std::size_t>{0});

        // A handle put away would name an entity nothing recreates.
        EXPECT_EQ(
            grid.buildings[0].building.walkers[0], kNullEntity);
        EXPECT_EQ(grid.walkers[0].walker.home, kNullEntity);
    }

    TEST_F(CityGridTest, CityGridOf_DropsALinkWhoseWalkerIsGone)
    {
        const auto home = putUp(Cell{4, 4}, BuildingKind::Farm);
        const auto walker = sendOut(Cell{5, 4}, home);
        world.commit();

        auto sent = world.get<Building>(home);
        sent.walkers[0] = walker;
        world.set<Building>(home, sent);
        world.destroy(walker);
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 1U);
        EXPECT_TRUE(grid.walkers.empty());
        EXPECT_FALSE(grid.buildings[0].walkers[0].has_value());
    }

    TEST_F(CityGridTest, RestoreCityGrid_PutsBackWhatWasTaken)
    {
        layPath(Cell{1, 1});
        const auto home = putUp(Cell{4, 4}, BuildingKind::Well);
        sendOut(Cell{1, 1}, home);
        world.commit();

        auto sent = world.get<Building>(home);
        sent.walkers[0] = *world.view<Walker, Cell>().begin();
        world.set<Building>(home, sent);
        world.commit();

        const auto taken = cityGridOf(world);
        restoreCityGrid(world, built, paths, taken);
        world.commit();

        EXPECT_EQ(cityGridOf(world), taken);
    }

    TEST_F(CityGridTest, RestoreCityGrid_RelinksBothEndsToLiveEntities)
    {
        CityGrid grid;
        grid.walkers.push_back(
            StoredWalker{
                .at = Cell{1, 1},
                .walker = Walker{.facing = Direction::North},
                .home = 0});
        grid.buildings.push_back(
            StoredBuilding{
                .at = Cell{2, 2},
                .building = Building{.kind = BuildingKind::Farm},
                .walkers = {std::optional<std::size_t>{0}}});

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto walker = *world.view<Walker, Cell>().begin();
        const auto building = *world.view<Building, Cell>().begin();

        EXPECT_EQ(world.get<Walker>(walker).home, building);
        EXPECT_EQ(world.get<Building>(building).walkers[0], walker);
        EXPECT_TRUE(world.alive(walker));
        EXPECT_TRUE(world.alive(building));
    }

    TEST_F(CityGridTest, RestoreCityGrid_LeavesTheUnlinkedUnlinked)
    {
        CityGrid grid;
        grid.walkers.push_back(
            StoredWalker{
                .at = Cell{1, 1}, .walker = Walker{}});
        grid.buildings.push_back(
            StoredBuilding{
                .at = Cell{2, 2}, .building = Building{}});

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto walker = *world.view<Walker, Cell>().begin();
        const auto building = *world.view<Building, Cell>().begin();

        EXPECT_EQ(world.get<Walker>(walker).home, kNullEntity);
        EXPECT_EQ(
            world.get<Building>(building).walkers[0], kNullEntity);
    }

    TEST_F(CityGridTest, RestoreCityGrid_DestroysWhatWasStanding)
    {
        layPath(Cell{1, 1});
        putUp(Cell{4, 4}, BuildingKind::House);
        sendOut(Cell{1, 1}, kNullEntity);
        world.commit();
        ASSERT_EQ(countOf(), 3U);

        restoreCityGrid(world, built, PathIndex{}, CityGrid{});
        world.commit();

        EXPECT_EQ(countOf(), 0U);
        EXPECT_EQ(built.size(), 0U);
    }

    TEST_F(CityGridTest, RestoreCityGrid_LaysOnePathEntityPerRoad)
    {
        PathIndex roads;
        roads.insert(Cell{3, 3});
        roads.insert(Cell{3, 4});

        restoreCityGrid(world, built, roads, CityGrid{});
        world.commit();

        const auto laid = world.view<Path, Cell>().size();
        EXPECT_EQ(laid, 2U);
    }

    TEST_F(CityGridTest, RestoreCityGrid_RebuildsTheBuildingIndex)
    {
        // Stale from whichever city was live before this one.
        (void)built.insert(
            Cell{9, 9}, antwika::game::footprintOf(BuildingKind::House));

        CityGrid grid;
        grid.buildings.push_back(
            StoredBuilding{
                .at = Cell{2, 2},
                .building = Building{.kind = BuildingKind::House}});

        restoreCityGrid(world, built, paths, grid);

        EXPECT_FALSE(built.has(Cell{9, 9}));
        EXPECT_TRUE(built.has(Cell{2, 2}));
    }

    [[nodiscard]] CityGrid populatedCityGrid()
    {
        CityGrid grid;
        grid.walkers = {
            StoredWalker{
                .at = {.x = 1, .y = 2},
                .walker =
                    Walker{.facing = Direction::West},
                .home = 0}};
        grid.buildings = {
            StoredBuilding{
                .at = {.x = 3, .y = 4},
                .building =
                    Building{
                        .kind = BuildingKind::Farm},
                .walkers = {std::optional<std::size_t>{0}}}};
        return grid;
    }

    TEST_F(CityGridTest, StoredWalkerEqualityComparesEveryField)
    {
        const auto base = populatedCityGrid().walkers.front();

        expectMemberCompared(
            base, [](StoredWalker &w) { w.at = Cell{.x = 9, .y = 9}; });
        expectMemberCompared(
            base,
            [](StoredWalker &w) { w.walker.facing = Direction::North; });
        expectMemberCompared(
            base, [](StoredWalker &w) { w.home = std::nullopt; });
    }

    TEST_F(CityGridTest, StoredBuildingEqualityComparesEveryField)
    {
        const auto base = populatedCityGrid().buildings.front();

        expectMemberCompared(
            base, [](StoredBuilding &b) { b.at = Cell{.x = 9, .y = 9}; });
        expectMemberCompared(
            base,
            [](StoredBuilding &b)
            { b.building.kind = BuildingKind::House; });
        expectMemberCompared(
            base, [](StoredBuilding &b) { b.walkers = {}; });
    }

    TEST_F(CityGridTest, EqualityComparesEveryField)
    {
        const auto base = populatedCityGrid();

        expectMemberCompared(
            base, [](CityGrid &g) { g.walkers.clear(); });
        expectMemberCompared(
            base, [](CityGrid &g) { g.buildings.clear(); });
    }

} // namespace
