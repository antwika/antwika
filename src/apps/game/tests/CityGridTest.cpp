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
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Ruin.hpp"
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
    using antwika::game::HousingLevel;
    using antwika::game::Production;
    using antwika::game::Resource;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::restoreCityGrid;
    using antwika::game::Employment;
    using antwika::game::JobHolding;
    using antwika::game::Staff;
    using antwika::game::StaffEntry;
    using antwika::game::StoredBuilding;
    using antwika::game::StoredEmployment;
    using antwika::game::StoredJob;
    using antwika::game::StoredStaff;
    using antwika::game::StoredStaffEntry;
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
        expectMemberCompared(
            base, [](StoredWalker &w) { w.attending = 3U; });
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
        expectMemberCompared(
            base,
            [](CityGrid &g)
            {
                g.ruins.push_back(antwika::game::StoredRuin{});
            });
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

    // The journeys, on exactly the errands' terms.
    TEST_F(CityGridTest, CityGridOf_TakesTheJourneys)
    {
        layPath(Cell{1, 1});
        const auto house = putUp(Cell{2, 2}, BuildingKind::House);
        const auto mover = sendOut(Cell{1, 1}, kNullEntity);
        const auto leaver = sendOut(Cell{1, 1}, kNullEntity);
        world.add<antwika::game::Journey>(
            mover,
            antwika::game::Journey{
                .towards = Cell{2, 2}, .house = house});
        world.add<antwika::game::Journey>(
            leaver, antwika::game::Journey{.towards = Cell{0, 1}});
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.walkers.size(), 2U);
        ASSERT_TRUE(grid.walkers[0].journey.has_value());
        EXPECT_EQ(grid.walkers[0].journey->towards, (Cell{2, 2}));
        EXPECT_EQ(grid.walkers[0].journey->house, kNullEntity);
        EXPECT_EQ(grid.walkers[0].joining, 0U);

        ASSERT_TRUE(grid.walkers[1].journey.has_value());
        EXPECT_FALSE(grid.walkers[1].joining.has_value());
    }

    TEST_F(CityGridTest, CityGridOf_ForgetsAJourneyNamingNobodyItKept)
    {
        layPath(Cell{1, 1});
        const auto mover = sendOut(Cell{1, 1}, kNullEntity);
        world.add<antwika::game::Journey>(
            mover,
            antwika::game::Journey{
                .towards = Cell{2, 2},
                .house = static_cast<Entity>(99)});
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.walkers.size(), 1U);
        EXPECT_FALSE(grid.walkers[0].joining.has_value());
    }

    TEST_F(CityGridTest, RestoreCityGrid_PutsTheJourneysBack)
    {
        paths.insert(Cell{1, 1});

        CityGrid grid;
        grid.walkers = {
            StoredWalker{
                .at = Cell{1, 1},
                .walker = Walker{},
                .journey =
                    antwika::game::Journey{.towards = Cell{2, 2}},
                .joining = 0U},
            StoredWalker{
                .at = Cell{1, 1},
                .walker = Walker{},
                .journey =
                    antwika::game::Journey{.towards = Cell{0, 1}}}};
        grid.buildings = {
            StoredBuilding{
                .at = Cell{2, 2},
                .building = Building{.kind = BuildingKind::House}}};

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.walkers.size(), 2U);
        ASSERT_TRUE(taken.walkers[0].journey.has_value());
        EXPECT_EQ(taken.walkers[0].journey->towards, (Cell{2, 2}));
        EXPECT_EQ(taken.walkers[0].joining, 0U);
        ASSERT_TRUE(taken.walkers[1].journey.has_value());
        EXPECT_FALSE(taken.walkers[1].joining.has_value());
    }

    TEST_F(CityGridTest, StoredWalkerEqualityComparesItsJourney)
    {
        StoredWalker base{.at = Cell{1, 1}, .walker = Walker{}};
        base.journey = antwika::game::Journey{};
        base.joining = 2U;

        expectMemberCompared(
            base, [](StoredWalker &w) { w.journey.reset(); });
        expectMemberCompared(
            base, [](StoredWalker &w) { w.joining.reset(); });
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


    TEST_F(CityGridTest, StoredBuildingEqualityComparesItsLedgers)
    {
        StoredBuilding base{.at = Cell{1, 1}, .building = Building{}};
        base.staff = StoredStaff{
            .entries = {StoredStaffEntry{.house = 1, .count = 3}},
            .ticksUntilDecay = 5};
        base.employment = StoredEmployment{
            .jobs = {StoredJob{.workplace = 0, .count = 3}},
            .ticksUntilDispatch = 7};

        expectMemberCompared(
            base, [](StoredBuilding &b) { b.staff.reset(); });
        expectMemberCompared(
            base,
            [](StoredBuilding &b) { b.staff->entries[0].count = 0; });
        expectMemberCompared(
            base, [](StoredBuilding &b) { b.employment.reset(); });
        expectMemberCompared(
            base,
            [](StoredBuilding &b) { b.employment->jobs[0].count = 0; });
    }

    // The two ledgers cross the switch with their links intact.
    // A city reopened unstaffed is a city whose walkers all stop.
    TEST_F(CityGridTest, CityGrid_CarriesTheLedgersAcrossACitySwitch)
    {
        const auto farm = putUp(Cell{2, 2}, BuildingKind::Farm);
        const auto home = putUp(Cell{6, 6}, BuildingKind::House);

        Staff staff;
        staff.sources[0] = StaffEntry{.house = home, .count = 2};
        staff.ticksUntilDecay = 9;
        antwika::game::setStaff(world, farm, staff);

        Employment employment;
        employment.jobs[0] = JobHolding{.workplace = farm, .count = 2};
        employment.ticksUntilDispatch = 11;
        antwika::game::setEmployment(world, home, employment);
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_EQ(grid.buildings.size(), 2U);
        ASSERT_TRUE(grid.buildings[0].staff.has_value());
        ASSERT_EQ(grid.buildings[0].staff->entries.size(), 1U);
        EXPECT_EQ(grid.buildings[0].staff->entries[0].house, 1U);
        EXPECT_EQ(grid.buildings[0].staff->entries[0].count, 2);
        ASSERT_TRUE(grid.buildings[1].employment.has_value());
        ASSERT_EQ(grid.buildings[1].employment->jobs.size(), 1U);
        EXPECT_EQ(grid.buildings[1].employment->jobs[0].workplace, 0U);

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.buildings.size(), 2U);
        EXPECT_EQ(taken.buildings[0].staff, grid.buildings[0].staff);
        EXPECT_EQ(
            taken.buildings[1].employment,
            grid.buildings[1].employment);
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

    // An empty entry, and one naming something that is no building.
    // Both are dropped on the way out rather than carried as noise.
    TEST_F(CityGridTest, CityGrid_DropsLedgerEntriesNamingNothing)
    {
        const auto farm = putUp(Cell{2, 2}, BuildingKind::Farm);
        const auto home = putUp(Cell{6, 6}, BuildingKind::House);

        const auto stray = world.create();
        world.add<Cell>(stray, Cell{1, 1});
        world.commit();

        Staff staff;
        staff.sources[0] = StaffEntry{.house = home, .count = 0};
        staff.sources[1] = StaffEntry{.house = stray, .count = 2};
        antwika::game::setStaff(world, farm, staff);

        Employment employment;
        employment.jobs[0] = JobHolding{.workplace = farm, .count = 0};
        employment.jobs[1] = JobHolding{.workplace = stray, .count = 2};
        antwika::game::setEmployment(world, home, employment);
        world.commit();

        const auto grid = cityGridOf(world);

        ASSERT_TRUE(grid.buildings[0].staff.has_value());
        EXPECT_TRUE(grid.buildings[0].staff->entries.empty());
        ASSERT_TRUE(grid.buildings[1].employment.has_value());
        EXPECT_TRUE(grid.buildings[1].employment->jobs.empty());
    }

    // A stored ledger longer than the live component has slots.
    // What fits is restored and the tail is dropped, deterministically.
    TEST_F(CityGridTest, RestoreCityGrid_ClampsALedgerToItsSlots)
    {
        CityGrid grid;
        grid.buildings.push_back(
            StoredBuilding{
                .at = Cell{2, 2},
                .building =
                    Building{.kind = BuildingKind::Farm}});
        grid.buildings.push_back(
            StoredBuilding{
                .at = Cell{6, 6},
                .building =
                    Building{.kind = BuildingKind::House}});

        StoredStaff staff;

        for (std::size_t extra = 0;
             extra < antwika::game::kMaxStaffSources + 2;
             ++extra)
        {
            staff.entries.push_back(
                StoredStaffEntry{.house = 1, .count = 1});
        }

        grid.buildings[0].staff = staff;

        StoredEmployment employment;

        for (std::size_t extra = 0;
             extra < antwika::game::kMaxJobs + 2;
             ++extra)
        {
            employment.jobs.push_back(
                StoredJob{.workplace = 0, .count = 1});
        }

        grid.buildings[1].employment = employment;

        restoreCityGrid(world, built, paths, grid);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_TRUE(taken.buildings[0].staff.has_value());
        EXPECT_EQ(
            taken.buildings[0].staff->entries.size(),
            antwika::game::kMaxStaffSources);
        ASSERT_TRUE(taken.buildings[1].employment.has_value());
        EXPECT_EQ(
            taken.buildings[1].employment->jobs.size(),
            antwika::game::kMaxJobs);
    }

    // A ruin is put away and put back exactly as a building is.
    // Its block goes back into the index.
    // So a reopened city still refuses a placement on debris.
    TEST_F(CityGridTest, CityGrid_CarriesTheRuinsAcrossACitySwitch)
    {
        const auto fire = world.create();
        world.add<Cell>(fire, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Ruin>(
            fire,
            antwika::game::Ruin{
                .kind = BuildingKind::Farm, .ticksUntilOut = 55});
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.ruins.size(), 1U);
        EXPECT_EQ(taken.ruins[0].at, (Cell{.x = 4, .y = 4}));
        EXPECT_EQ(taken.ruins[0].ruin.kind, BuildingKind::Farm);
        EXPECT_EQ(taken.ruins[0].ruin.ticksUntilOut, 55);

        restoreCityGrid(world, built, paths, taken);
        world.commit();

        EXPECT_EQ(cityGridOf(world), taken);
        EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
        EXPECT_TRUE(built.has(Cell{.x = 5, .y = 5}));
    }

    // The call crosses the switch as an index, exactly as a home does.
    TEST_F(CityGridTest, CityGrid_CarriesAFiremansCallAcrossACitySwitch)
    {
        const auto fire = world.create();
        world.add<Cell>(fire, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Ruin>(
            fire, antwika::game::Ruin{.kind = BuildingKind::House});

        const auto fireman = world.create();
        world.add<Cell>(fireman, Cell{.x = 1, .y = 1});
        world.add<Walker>(
            fireman,
            Walker{.kind = antwika::game::WalkerKind::Fireman});
        world.add<antwika::game::FireCall>(
            fireman, antwika::game::FireCall{.target = fire});
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.walkers.size(), 1U);
        EXPECT_EQ(taken.walkers[0].attending, 0U);

        restoreCityGrid(world, built, paths, taken);
        world.commit();

        // The relinked call names the live ruin, whatever its id now.
        const auto walkers =
            world.view<Walker, antwika::game::FireCall>();
        ASSERT_EQ(walkers.size(), 1U);

        const auto target = world
            .get<antwika::game::FireCall>(*walkers.begin())
            .target;
        EXPECT_TRUE(world.has<antwika::game::Ruin>(target));
    }

    // A call whose ruin died the same tick is dropped on the way out.
    TEST_F(CityGridTest, CityGridOf_ForgetsACallNamingNoRuinItKept)
    {
        const auto fire = world.create();
        world.add<Cell>(fire, Cell{.x = 4, .y = 4});
        world.add<antwika::game::Ruin>(
            fire, antwika::game::Ruin{.kind = BuildingKind::House});

        const auto fireman = world.create();
        world.add<Cell>(fireman, Cell{.x = 1, .y = 1});
        world.add<Walker>(
            fireman,
            Walker{.kind = antwika::game::WalkerKind::Fireman});
        world.add<antwika::game::FireCall>(
            fireman, antwika::game::FireCall{.target = fire});
        world.destroy(fire);
        world.commit();

        const auto taken = cityGridOf(world);

        ASSERT_EQ(taken.walkers.size(), 1U);
        EXPECT_FALSE(taken.walkers[0].attending.has_value());
    }

    TEST_F(CityGridTest, StoredRuinEqualityComparesEveryField)
    {
        const antwika::game::StoredRuin base{
            .at = Cell{.x = 4, .y = 4},
            .ruin = antwika::game::Ruin{.kind = BuildingKind::Farm}};

        expectMemberCompared(
            base,
            [](antwika::game::StoredRuin &r)
            { r.at = Cell{.x = 5, .y = 5}; });
        expectMemberCompared(
            base,
            [](antwika::game::StoredRuin &r)
            { r.ruin.state = antwika::game::RuinState::Debris; });
        expectMemberCompared(
            base,
            [](antwika::game::StoredRuin &r)
            { r.ruin.kind = BuildingKind::House; });
        expectMemberCompared(
            base,
            [](antwika::game::StoredRuin &r)
            { r.ruin.ticksUntilOut = 1; });
    }
