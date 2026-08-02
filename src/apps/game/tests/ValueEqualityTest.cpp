#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Terrain.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"

// A defaulted operator== is one comparison per member.
// A test that varies one member proves only that member is in it.
// These vary each in turn.
// A member left out of a comparison then fails here.
// Rather than letting a run and its replay agree for a wrong reason.
namespace
{

    using antwika::game::BuildGhost;
    using antwika::game::Building;
    using antwika::game::BuildingSprite;
    using antwika::game::BuildingView;
    using antwika::game::BuildingKind;
    using antwika::game::BuildTool;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::Direction;
    using antwika::game::Errand;
    using antwika::game::ErrandLeg;
    using antwika::game::Production;
    using antwika::game::Resource;
    using antwika::game::GameState;
    using antwika::game::GameSummary;
    using antwika::game::GridExtent;
    using antwika::game::HoverReadout;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::HousingRequirement;
    using antwika::game::RoadPlan;
    using antwika::game::SaveGame;
    using antwika::game::SceneSnapshot;
    using antwika::game::Terrain;
    using antwika::game::WalkerSprite;
    using antwika::game::WalkerView;
    using antwika::game::WorldMap;
    using antwika::game::WorldMapSnapshot;

    // Varying one member at a time, from a value that differs in none.
    template <typename T, typename Mutate>
    void expectMemberCompared(const T &base, Mutate mutate)
    {
        T changed = base;
        mutate(changed);

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, base);
    }

    TEST(BuildGhostTest, EqualityComparesEveryField)
    {
        const BuildGhost base{
            .at = {.x = 1, .y = 2},
            .tool = BuildTool::House,
            .visible = true};

        expectMemberCompared(
            base, [](BuildGhost &g) { g.at = Cell{.x = 9, .y = 9}; });
        expectMemberCompared(
            base, [](BuildGhost &g) { g.tool = BuildTool::Farm; });
        expectMemberCompared(
            base, [](BuildGhost &g) { g.visible = false; });
    }

    TEST(SceneSnapshotTest, BuildingViewEqualityComparesEveryField)
    {
        const BuildingView base{
            .at = {.x = 3, .y = 4}, .kind = BuildingKind::Farm};

        expectMemberCompared(
            base, [](BuildingView &b) { b.at = Cell{.x = 0, .y = 0}; });
        expectMemberCompared(
            base, [](BuildingView &b) { b.kind = BuildingKind::Well; });
        expectMemberCompared(
            base, [](BuildingView &b) { b.coverage[0] = 99; });
        expectMemberCompared(
            base, [](BuildingView &b) { b.coverage[3] = 99; });
        expectMemberCompared(
            base,
            [](BuildingView &b) { b.level = HousingLevel::Cottage; });
    }

    TEST(SceneSnapshotTest, BuildingSpriteEqualityComparesEveryField)
    {
        const BuildingSprite base{
            .at = {.x = 3, .y = 4},
            .kind = BuildingKind::House,
            .stock = {10, 20}};

        expectMemberCompared(
            base, [](BuildingSprite &b) { b.at = Cell{.x = 0, .y = 0}; });
        expectMemberCompared(
            base,
            [](BuildingSprite &b) { b.kind = BuildingKind::Well; });
        expectMemberCompared(
            base, [](BuildingSprite &b) { b.stock[0] = 99; });
        expectMemberCompared(
            base, [](BuildingSprite &b) { b.stock[1] = 99; });
        expectMemberCompared(
            base, [](BuildingSprite &b) { b.coverage[0] = 99; });
        expectMemberCompared(
            base, [](BuildingSprite &b) { b.coverage[3] = 99; });
        expectMemberCompared(
            base,
            [](BuildingSprite &b) { b.level = HousingLevel::Cottage; });
    }

    [[nodiscard]] SceneSnapshot populatedSnapshot()
    {
        return SceneSnapshot{
            .camera = Camera(antwika::gfx::Point{.x = 4, .y = 5}, 1),
            .extent = GridExtent{.width = 8, .height = 8},
            .paths = {Cell{.x = 1, .y = 1}},
            .walkers = {WalkerSprite{.at = {.x = 2, .y = 2}}},
            .buildings =
                {BuildingSprite{
                    .at = {.x = 3, .y = 3}, .kind = BuildingKind::House}},
            .plan = RoadPlan{.cells = {Cell{.x = 5, .y = 5}}},
            .ghost = BuildGhost{.at = {.x = 4, .y = 4}},
            .hover = HoverReadout{
                .anchor = antwika::gfx::Point{.x = 6, .y = 7}}};
    }

    TEST(SceneSnapshotTest, EqualityComparesEveryField)
    {
        const auto base = populatedSnapshot();

        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.camera = Camera(); });
        expectMemberCompared(
            base,
            [](SceneSnapshot &s)
            { s.extent = GridExtent{.width = 2, .height = 2}; });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.paths.clear(); });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.walkers.clear(); });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.buildings.clear(); });
        expectMemberCompared(
            base,
            [](SceneSnapshot &s) { s.ghost.visible = !s.ghost.visible; });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.hover.anchor.x = 99; });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.plan.cells.clear(); });
        expectMemberCompared(
            base, [](SceneSnapshot &s) { s.plan.valid = !s.plan.valid; });
    }

    [[nodiscard]] GameSummary populatedSummary()
    {
        return GameSummary{
            .state = GameState{.ticksProcessed = 3, .score = 4},
            .paths = {Cell{.x = 1, .y = 1}},
            .walkers = {WalkerView{.at = {.x = 2, .y = 2}}},
            .buildings =
                {BuildingView{
                    .at = {.x = 3, .y = 3}, .kind = BuildingKind::House}},
            .camera = Camera(antwika::gfx::Point{.x = 4, .y = 5}, 1)};
    }

    TEST(GameSummaryTest, EqualityComparesEveryField)
    {
        const auto base = populatedSummary();

        expectMemberCompared(
            base, [](GameSummary &s) { s.state.score = 99; });
        expectMemberCompared(
            base, [](GameSummary &s) { s.paths.clear(); });
        expectMemberCompared(
            base, [](GameSummary &s) { s.walkers.clear(); });
        expectMemberCompared(
            base, [](GameSummary &s) { s.buildings.clear(); });
        expectMemberCompared(
            base, [](GameSummary &s) { s.camera = Camera(); });
    }

    [[nodiscard]] SaveGame populatedSave()
    {
        SaveGame save;
        save.state = GameState{.ticksProcessed = 3, .score = 4};
        save.extent = GridExtent{.width = 8, .height = 8};
        save.camera = Camera(antwika::gfx::Point{.x = 4, .y = 5}, 1);
        save.paths = {Cell{.x = 1, .y = 1}};
        save.walkers = {
            antwika::game::SavedWalker{
                .at = {.x = 2, .y = 2}, .facing = Direction::West}};
        save.seed = 7;
        return save;
    }

    TEST(SaveGameTest, EqualityComparesEveryField)
    {
        const auto base = populatedSave();

        expectMemberCompared(
            base, [](SaveGame &s) { s.state.ticksProcessed = 99; });
        expectMemberCompared(
            base,
            [](SaveGame &s)
            { s.extent = GridExtent{.width = 2, .height = 2}; });
        expectMemberCompared(
            base, [](SaveGame &s) { s.camera = Camera(); });
        expectMemberCompared(
            base, [](SaveGame &s) { s.paths.clear(); });
        expectMemberCompared(
            base, [](SaveGame &s) { s.walkers.clear(); });
        expectMemberCompared(base, [](SaveGame &s) { s.seed = 99; });
    }

    [[nodiscard]] WorldMap populatedWorld()
    {
        WorldMap world;
        world.width = 2;
        world.height = 2;
        world.tiles = {
            Terrain::Plains,
            Terrain::Forest,
            Terrain::Hills,
            Terrain::Water};
        world.cities = {0, 1, 2, 3};
        return world;
    }

    TEST(WorldMapTest, EqualityComparesEveryField)
    {
        const auto base = populatedWorld();

        expectMemberCompared(base, [](WorldMap &w) { w.width = 9; });
        expectMemberCompared(base, [](WorldMap &w) { w.height = 9; });
        expectMemberCompared(
            base, [](WorldMap &w) { w.tiles[0] = Terrain::Mountain; });
        expectMemberCompared(base, [](WorldMap &w) { w.cities[3] = 0; });
    }

    TEST(WorldMapSceneTest, SnapshotEqualityComparesEveryField)
    {
        WorldMapSnapshot base;
        base.width = 2;
        base.height = 2;
        base.tiles = {
            Terrain::Plains,
            Terrain::Forest,
            Terrain::Hills,
            Terrain::Water};
        base.cities = {
            Cell{.x = 0, .y = 0},
            Cell{.x = 1, .y = 0},
            Cell{.x = 0, .y = 1},
            Cell{.x = 1, .y = 1}};

        expectMemberCompared(
            base, [](WorldMapSnapshot &s) { s.width = 9; });
        expectMemberCompared(
            base, [](WorldMapSnapshot &s) { s.height = 9; });
        expectMemberCompared(
            base,
            [](WorldMapSnapshot &s) { s.tiles[0] = Terrain::Mountain; });
        expectMemberCompared(
            base,
            [](WorldMapSnapshot &s)
            { s.cities[3] = Cell{.x = 5, .y = 5}; });
    }

    // The component itself, which no other value test reaches.
    TEST(BuildingTest, EqualityComparesEveryField)
    {
        const Building base{
            .kind = BuildingKind::Farm,
            .stock = {1, 2, 3},
            .risk = 3,
            .ticksUntilSpawn = 4,
            .ticksUntilDrain = 5,
            .ticksUntilRisk = 6,
            .walkers = {
                antwika::ecs::Entity{7}, antwika::ecs::Entity{8}}};

        expectMemberCompared(
            base, [](Building &b) { b.kind = BuildingKind::House; });
        expectMemberCompared(base, [](Building &b) { b.stock[0] = 99; });
        expectMemberCompared(base, [](Building &b) { b.risk = 99; });
        expectMemberCompared(
            base, [](Building &b) { b.ticksUntilSpawn = 99; });
        expectMemberCompared(
            base, [](Building &b) { b.ticksUntilDrain = 99; });
        expectMemberCompared(
            base, [](Building &b) { b.ticksUntilRisk = 99; });
        expectMemberCompared(
            base,
            [](Building &b)
            { b.walkers[0] = antwika::ecs::kNullEntity; });
        expectMemberCompared(
            base,
            [](Building &b)
            { b.walkers[1] = antwika::ecs::kNullEntity; });
    }


    TEST(ErrandComponentTest, EqualityComparesEveryField)
    {
        const Errand base{
            .destination = static_cast<antwika::ecs::Entity>(3),
            .carrying = Resource::Clay,
            .leg = ErrandLeg::Returning};

        expectMemberCompared(
            base,
            [](Errand &e)
            { e.destination = antwika::ecs::kNullEntity; });
        expectMemberCompared(
            base, [](Errand &e) { e.carrying = Resource::Food; });
        expectMemberCompared(
            base, [](Errand &e) { e.leg = ErrandLeg::Outbound; });
    }

    TEST(ProductionComponentTest, EqualityComparesItsCountdown)
    {
        const Production base{.ticksUntilOutput = 5};

        expectMemberCompared(
            base, [](Production &p) { p.ticksUntilOutput = 0; });
    }

    TEST(HousingRequirementTest, EqualityComparesEveryField)
    {
        const HousingRequirement base{
            .desirability = 2,
            .services = {true, false, true, false},
            .goods = {10, 0, 0},
            .populationCapacity = 8};

        expectMemberCompared(
            base, [](HousingRequirement &r) { r.desirability = 0; });
        expectMemberCompared(
            base,
            [](HousingRequirement &r) { r.services[0] = false; });
        expectMemberCompared(
            base, [](HousingRequirement &r) { r.services[2] = false; });
        expectMemberCompared(
            base, [](HousingRequirement &r) { r.goods[0] = 0; });
        expectMemberCompared(
            base,
            [](HousingRequirement &r) { r.populationCapacity = 0; });
    }

    TEST(HouseholdComponentTest, EqualityComparesEveryField)
    {
        const Household base{
            .level = HousingLevel::Hovel,
            .ticksUntilEvolve = 7,
            .ticksUntilDevolve = 9,
            .population = 11};

        expectMemberCompared(
            base, [](Household &h) { h.level = HousingLevel::Tent; });
        expectMemberCompared(
            base, [](Household &h) { h.ticksUntilEvolve = 0; });
        expectMemberCompared(
            base, [](Household &h) { h.ticksUntilDevolve = 0; });
        expectMemberCompared(
            base, [](Household &h) { h.population = 0; });
    }

} // namespace
