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
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Workforce.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
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
    using antwika::game::Coverage;
    using antwika::game::coverageOf;
    using antwika::game::Direction;
    using antwika::game::Errand;
    using antwika::game::ErrandLeg;
    using antwika::game::Household;
    using antwika::game::Workforce;
    using antwika::game::HousingLevel;
    using antwika::game::Production;
    using antwika::game::Resource;
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
        expectMemberCompared(
            base,
            [](StoredBuilding &b) { b.coverage.ticksLeft[0] = 99; });
    }

    // A city put away and opened again is served exactly as it was.
    TEST_F(CityGridTest, CityGrid_CarriesCoverageAcrossACitySwitch)
    {
        const auto well = putUp(Cell{.x = 2, .y = 2}, BuildingKind::Well);
        world.commit();
        antwika::game::setCoverage(
            world, well, Coverage{.ticksLeft = {5, 6, 7, 8}});
        world.commit();

        const auto stored = cityGridOf(world);

        ASSERT_EQ(stored.buildings.size(), 1U);
        EXPECT_EQ(
            stored.buildings[0].coverage,
            (Coverage{.ticksLeft = {5, 6, 7, 8}}));

        restoreCityGrid(world, built, paths, stored);
        world.commit();

        const auto entities = world.view<Building, Cell>();
        ASSERT_EQ(entities.size(), 1U);
        EXPECT_EQ(
            coverageOf(world, *entities.begin()),
            (Coverage{.ticksLeft = {5, 6, 7, 8}}));
    }

    // An absent component already means uncovered.
    // So a city nobody served comes back with none at all.
    TEST_F(CityGridTest, CityGrid_PutsBackNoCoverageWhereThereWasNone)
    {
        (void)putUp(Cell{.x = 2, .y = 2}, BuildingKind::Well);
        world.commit();

        const auto stored = cityGridOf(world);

        restoreCityGrid(world, built, paths, stored);
        world.commit();

        const auto entities = world.view<Building, Cell>();
        ASSERT_EQ(entities.size(), 1U);
        EXPECT_FALSE(world.has<Coverage>(*entities.begin()));
    }

    TEST_F(CityGridTest, EqualityComparesEveryField)
    {
        const auto base = populatedCityGrid();

        expectMemberCompared(
            base, [](CityGrid &g) { g.walkers.clear(); });
        expectMemberCompared(
            base, [](CityGrid &g) { g.buildings.clear(); });
    }


    // A city put away and opened again is the same city.
    // Every countdown comes along for the reason Building's three do.
    TEST_F(CityGridTest, CityGridOf_TakesTheErrandsAndTheCountdowns)
    {
        layPath(Cell{1, 1});
        const auto farm = putUp(Cell{2, 2}, BuildingKind::Farm);
        const auto store = putUp(Cell{6, 6}, BuildingKind::Storage);
        const auto cart = sendOut(Cell{1, 1}, farm);
        world.add<Production>(farm, Production{.ticksUntilOutput = 9});
        world.add<Errand>(
            cart,
            Errand{
                .destination = store,
                .carrying = Resource::Clay,
                .leg = ErrandLeg::Returning});
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.walkers.size(), 1U);
        ASSERT_TRUE(grid.walkers[0].errand.has_value());
        EXPECT_EQ(grid.walkers[0].errand->carrying, Resource::Clay);
        EXPECT_EQ(grid.walkers[0].errand->leg, ErrandLeg::Returning);
        EXPECT_EQ(grid.walkers[0].errand->destination, kNullEntity);
        EXPECT_EQ(grid.walkers[0].destination, 1U);

        ASSERT_EQ(grid.buildings.size(), 2U);
        ASSERT_TRUE(grid.buildings[0].production.has_value());
        EXPECT_EQ(grid.buildings[0].production->ticksUntilOutput, 9);
        EXPECT_FALSE(grid.buildings[1].production.has_value());
    }

    TEST_F(CityGridTest, CityGridOf_ForgetsAnErrandNamingNobodyItKept)
    {
        layPath(Cell{1, 1});
        const auto cart = sendOut(Cell{1, 1}, kNullEntity);
        world.add<Errand>(
            cart,
            Errand{
                .destination = static_cast<Entity>(99),
                .carrying = Resource::Food});
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.walkers.size(), 1U);
        EXPECT_FALSE(grid.walkers[0].destination.has_value());
    }

    TEST_F(CityGridTest, RestoreCityGrid_PutsTheErrandsAndCountdownsBack)
    {
        paths.insert(Cell{1, 1});

        CityGrid grid;
        grid.walkers = {
            StoredWalker{
                .at = Cell{1, 1},
                .walker = Walker{},
                .errand =
                    Errand{
                        .carrying = Resource::Pottery,
                        .leg = ErrandLeg::Returning},
                .destination = 1U},
            StoredWalker{.at = Cell{1, 1}, .walker = Walker{}}};
        grid.buildings = {
            StoredBuilding{
                .at = Cell{2, 2},
                .building = Building{.kind = BuildingKind::Farm},
                .production = Production{.ticksUntilOutput = 4}},
            StoredBuilding{
                .at = Cell{6, 6},
                .building = Building{.kind = BuildingKind::Storage}}};

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.walkers.size(), 2U);
        ASSERT_TRUE(taken.walkers[0].errand.has_value());
        EXPECT_EQ(taken.walkers[0].errand->carrying, Resource::Pottery);
        EXPECT_EQ(taken.walkers[0].destination, 1U);
        EXPECT_FALSE(taken.walkers[1].errand.has_value());

        ASSERT_EQ(taken.buildings.size(), 2U);
        EXPECT_EQ(taken.buildings[0].production, grid.buildings[0].production);
        EXPECT_FALSE(taken.buildings[1].production.has_value());
    }

    TEST_F(CityGridTest, StoredWalkerEqualityComparesItsErrand)
    {
        StoredWalker base{.at = Cell{1, 1}, .walker = Walker{}};
        base.errand = Errand{};
        base.destination = 2U;

        expectMemberCompared(
            base, [](StoredWalker &w) { w.errand.reset(); });
        expectMemberCompared(
            base, [](StoredWalker &w) { w.destination.reset(); });
    }

    TEST_F(CityGridTest, StoredBuildingEqualityComparesItsCountdown)
    {
        StoredBuilding base{.at = Cell{1, 1}, .building = Building{}};
        base.production = Production{};

        expectMemberCompared(
            base, [](StoredBuilding &b) { b.production.reset(); });
    }

    TEST_F(CityGridTest, StoredBuildingEqualityComparesItsHousehold)
    {
        StoredBuilding base{.at = Cell{1, 1}, .building = Building{}};
        base.household = Household{.level = HousingLevel::Hovel};

        expectMemberCompared(
            base, [](StoredBuilding &b) { b.household.reset(); });
        expectMemberCompared(
            base,
            [](StoredBuilding &b)
            { b.household->level = HousingLevel::Tent; });
    }

    // A district built up over a run is not lost to the world map.
    // And the countdowns come along for every other one's reason.
    TEST_F(CityGridTest, CityGrid_CarriesAHouseholdAcrossACitySwitch)
    {
        const auto house = putUp(Cell{2, 2}, BuildingKind::House);
        const auto well = putUp(Cell{6, 6}, BuildingKind::Well);
        antwika::game::setHousehold(
            world,
            house,
            Household{
                .level = HousingLevel::Hovel,
                .ticksUntilEvolve = 12,
                .ticksUntilDevolve = 34,
                .population = 7});
        world.commit();
        EXPECT_TRUE(world.alive(well));

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 2U);
        ASSERT_TRUE(grid.buildings[0].household.has_value());
        EXPECT_EQ(grid.buildings[0].household->level, HousingLevel::Hovel);
        EXPECT_EQ(grid.buildings[0].household->ticksUntilEvolve, 12);
        EXPECT_EQ(grid.buildings[0].household->ticksUntilDevolve, 34);
        EXPECT_EQ(grid.buildings[0].household->population, 7);
        EXPECT_FALSE(grid.buildings[1].household.has_value());

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.buildings.size(), 2U);
        EXPECT_EQ(taken.buildings[0].household, grid.buildings[0].household);
        EXPECT_FALSE(taken.buildings[1].household.has_value());
    }


    TEST_F(CityGridTest, StoredBuildingEqualityComparesItsWorkforce)
    {
        StoredBuilding base{.at = Cell{1, 1}, .building = Building{}};
        base.workforce = Workforce{.employed = 3};

        expectMemberCompared(
            base, [](StoredBuilding &b) { b.workforce.reset(); });
        expectMemberCompared(
            base, [](StoredBuilding &b) { b.workforce->employed = 0; });
    }

    // An absent Workforce means fully staffed rather than empty.
    // So a city reopened having lost one is a city that speeds up.
    TEST_F(CityGridTest, CityGrid_CarriesAWorkforceAcrossACitySwitch)
    {
        const auto farm = putUp(Cell{2, 2}, BuildingKind::Farm);
        putUp(Cell{6, 6}, BuildingKind::Well);
        antwika::game::setWorkforce(
            world, farm, Workforce{.employed = 2});
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 2U);
        ASSERT_TRUE(grid.buildings[0].workforce.has_value());
        EXPECT_EQ(grid.buildings[0].workforce->employed, 2);
        EXPECT_FALSE(grid.buildings[1].workforce.has_value());

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.buildings.size(), 2U);
        EXPECT_EQ(
            taken.buildings[0].workforce, grid.buildings[0].workforce);
        EXPECT_FALSE(taken.buildings[1].workforce.has_value());
    }

    TEST_F(CityGridTest, RestoreCityGrid_PutsBackAnErrandBoundNowhere)
    {
        paths.insert(Cell{1, 1});

        CityGrid grid;
        grid.walkers = {StoredWalker{
            .at = Cell{1, 1},
            .walker = Walker{},
            .errand = Errand{.carrying = Resource::Clay}}};

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_TRUE(taken.walkers[0].errand.has_value());
        EXPECT_EQ(taken.walkers[0].errand->destination, kNullEntity);
        EXPECT_FALSE(taken.walkers[0].destination.has_value());
    }


    // Opening a city is showing that city, not merging two.
    // So the errands and countdowns of the one it replaces go with it.
    TEST_F(CityGridTest, RestoreCityGrid_TakesTheLastCitysErrandsWithIt)
    {
        paths.insert(Cell{1, 1});

        CityGrid first;
        first.walkers = {
            StoredWalker{
                .at = Cell{1, 1},
                .walker = Walker{},
                .errand = Errand{.carrying = Resource::Clay}},
            StoredWalker{.at = Cell{1, 1}, .walker = Walker{}}};
        first.buildings = {
            StoredBuilding{
                .at = Cell{2, 2},
                .building = Building{.kind = BuildingKind::Farm},
                .production = Production{.ticksUntilOutput = 3}},
            StoredBuilding{
                .at = Cell{6, 6},
                .building = Building{.kind = BuildingKind::Storage}}};

        restoreCityGrid(world, built, paths, first);
        world.commit();

        restoreCityGrid(world, built, paths, CityGrid{});
        world.commit();

        EXPECT_EQ(cityGridOf(world), CityGrid{});
    }

} // namespace
