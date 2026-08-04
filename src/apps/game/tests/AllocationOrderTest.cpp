#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/SpawnSystem.hpp"

/**
 * @file
 * @brief The increment's one cross-cutting determinism test.
 *
 * **It builds one city twice, in two different creation orders, runs both
 * for the same number of ticks and compares what they came to.** Nothing
 * else in this suite would catch a system that quietly read
 * `ecs::View`'s order: a View iterates "whichever storage has the fewest
 * entities", which is stable for a given history -- so a replay of one
 * run agrees with it -- and which is not a property of the city that
 * anybody can name. Two histories that built the same city in different
 * orders are the one thing that tells them apart.
 *
 * The city is restored through SessionStore rather than clicked
 * together, because the order the buildings are *created* in is the whole
 * subject: restoreCityGrid() creates them in the order the array holds
 * them, so reversing the array is exactly the variable being changed and
 * nothing else moves with it. Clicking would move the tick each building
 * was placed on as well, and every countdown with it.
 */
namespace
{
    using antwika::event::Event;
    using antwika::event::mocks::MockEventSink;
    using antwika::event::TickEvent;
    using antwika::game::AppMode;
    using antwika::game::AppModeState;
    using antwika::game::BuildingKind;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::GameSummary;
    using antwika::game::GridExtent;
    using antwika::game::Household;
    using antwika::game::PathIndex;
    using antwika::game::SavedBuilding;
    using antwika::game::SaveGame;
    using antwika::input::InputEventCodec;
    using antwika::log::mocks::MockLogger;
    using antwika::replay::ReplaySource;
    using ::testing::NiceMock;

    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kTicks = 200;

    // Long enough for a walker to be out, too short for one to go home.
    // kRoamingSteps is 32 and a step is two ticks.
    // So nobody the cap let through has left yet.
    constexpr antwika::time::Tick kCappedTicks = 20;

    // One house of people, and three farms wanting more than there are.
    // Which farm goes short is the answer this test is about.
    // And it must not depend on which of them was built first.
    [[nodiscard]] std::vector<SavedBuilding> cityBuildings()
    {
        return {
            // Watered for the whole run, since a dry house sheds.
            // The city must be short of people for labour's reason.
            SavedBuilding{
                .at = Cell{.x = 1, .y = 5},
                .kind = BuildingKind::House,
                .stock = {antwika::game::kStockOnCompletion, 0, 0},
                .coverage = {antwika::game::kCoverageFull, 0},
                .household = Household{.population = 6}},
            SavedBuilding{
                .at = Cell{.x = 3, .y = 5}, .kind = BuildingKind::Farm},
            SavedBuilding{
                .at = Cell{.x = 6, .y = 5}, .kind = BuildingKind::Farm},
            SavedBuilding{
                .at = Cell{.x = 9, .y = 5}, .kind = BuildingKind::Farm}};
    }

    [[nodiscard]] SaveGame cityIn(const std::vector<std::size_t> &order)
    {
        SaveGame save;
        save.extent = kExtent;

        for (std::int32_t x = 1; x <= 11; ++x)
        {
            save.paths.push_back(Cell{.x = x, .y = 4});
        }

        const auto buildings = cityBuildings();

        for (const auto index : order)
        {
            save.buildings.push_back(buildings[index]);
        }

        return save;
    }

    [[nodiscard]] GameSummary runCity(
        const SaveGame &start, const antwika::time::Tick ticks = kTicks)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;

        std::vector<TickEvent> script{
            TickEvent{
                .tick = ticks - 2,
                .event = Event{.name = antwika::engine::events::kStop}}};
        ReplaySource source(script);

        return antwika::game::bootstrap(
            antwika::game::GameWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = start.extent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .maxTicks = ticks,
                .start = start});
    }

    // The second city, and the second limited amount.
    // Labour is split between workplaces; kWalkerLimit is split too.
    // A city sending more than the cap allows is that contention.
    //
    // Every one of these is a well.
    // One cell, one road beside it, and a saved ticksUntilSpawn of none.
    // So all of them ask on the very first tick.
    // And the cap has to turn some of them down.
    constexpr GridExtent kWideExtent{.width = 32, .height = 32};
    constexpr std::int32_t kWellRows = 3;
    constexpr std::int32_t kWellsPerRow = 32;

    // Past kWalkerLimit on purpose.
    // And by enough that one walking home cannot end the contention.
    static_assert(
        static_cast<std::size_t>(kWellRows * kWellsPerRow)
        > antwika::game::kWalkerLimit + 8);

    [[nodiscard]] std::vector<SavedBuilding> cappedCityBuildings()
    {
        std::vector<SavedBuilding> buildings;

        for (std::int32_t row = 0; row < kWellRows; ++row)
        {
            for (std::int32_t x = 0; x < kWellsPerRow; ++x)
            {
                buildings.push_back(SavedBuilding{
                    .at = Cell{.x = x, .y = 2 * row + 1},
                    .kind = BuildingKind::Well});
            }
        }

        // People enough for every well.
        // So the labour split turns nobody down and only the cap does.
        buildings.push_back(SavedBuilding{
            .at = Cell{.x = 0, .y = 9},
            .kind = BuildingKind::House,
            .stock = {antwika::game::kStockOnCompletion, 0, 0},
            .household = Household{
                .population = kWellRows * kWellsPerRow}});

        return buildings;
    }

    // The same city, with the buildings handed over in a given order.
    // reversed() and shuffled() below are the whole experiment.
    [[nodiscard]] SaveGame cappedCityIn(
        const std::vector<std::size_t> &order)
    {
        SaveGame save;
        save.extent = kWideExtent;

        for (std::int32_t row = 0; row <= kWellRows; ++row)
        {
            for (std::int32_t x = 0; x < kWellsPerRow; ++x)
            {
                save.paths.push_back(Cell{.x = x, .y = 2 * row});
            }
        }

        const auto buildings = cappedCityBuildings();

        for (const auto index : order)
        {
            save.buildings.push_back(buildings[index]);
        }

        return save;
    }

    [[nodiscard]] std::vector<std::size_t> inOrder()
    {
        std::vector<std::size_t> order(cappedCityBuildings().size());
        std::ranges::generate(
            order, [next = std::size_t{0}]() mutable { return next++; });

        return order;
    }

    [[nodiscard]] std::vector<std::size_t> reversed()
    {
        auto order = inOrder();
        std::ranges::reverse(order);

        return order;
    }

    // Every third one first, then the rest.
    // A shuffle written down rather than drawn.
    // So it is the same city on every run, and a failure reproduces.
    [[nodiscard]] std::vector<std::size_t> shuffled()
    {
        std::vector<std::size_t> order;
        const auto count = cappedCityBuildings().size();

        for (std::size_t start = 0; start < 3; ++start)
        {
            for (std::size_t index = start; index < count; index += 3)
            {
                order.push_back(index);
            }
        }

        return order;
    }

    // The summary lists what stands in the world's own order.
    // Which *is* the creation order.
    // So that member is the one thing two orders may differ in.
    // Sorting it asks "the same things, wherever they were listed".
    // Which is the claim this file is making.
    [[nodiscard]] GameSummary sorted(GameSummary summary)
    {
        std::ranges::sort(
            summary.buildings,
            [](const auto &left, const auto &right)
            { return left.at < right.at; });

        std::ranges::sort(
            summary.walkers,
            [](const auto &left, const auto &right)
            {
                return left.at != right.at ? left.at < right.at
                                           : left.facing < right.facing;
            });

        return summary;
    }
} // namespace

// Two runs that both did nothing would agree for the wrong reason.
// So this one has to be genuinely short of people first.
TEST(AllocationOrderTest, TheCityActuallyRunsShortOfPeople)
{
    const auto summary = runCity(cityIn({0, 1, 2, 3}));

    ASSERT_EQ(summary.buildings.size(), 4U);
    EXPECT_GT(summary.ratings.population, 0);
    EXPECT_GT(summary.ratings.employment, 0);
    EXPECT_LT(summary.ratings.employment, 100)
        << "every job was filled, so nothing was contended";
}

TEST(AllocationOrderTest, TheSameCityBuiltBackwardsComesToTheSameThing)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));
    const auto backwards = runCity(cityIn({3, 2, 1, 0}));

    EXPECT_EQ(sorted(backwards), sorted(forwards));
}

TEST(AllocationOrderTest, TheSameCityBuiltShuffledComesToTheSameThing)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));
    const auto shuffled = runCity(cityIn({2, 0, 3, 1}));

    EXPECT_EQ(sorted(shuffled), sorted(forwards));
}

// The ratings are what would catch an allocation off a view's order.
// So they are worth asserting on their own as well.
TEST(AllocationOrderTest, TheRatingsAreIdenticalUnderEveryCreationOrder)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));

    EXPECT_EQ(runCity(cityIn({3, 2, 1, 0})).ratings, forwards.ratings);
    EXPECT_EQ(runCity(cityIn({1, 3, 0, 2})).ratings, forwards.ratings);
}

// The same argument, over the other limited amount there is.
// Two runs that both sent everybody would agree for the wrong reason.
// So this one has to be genuinely short of slots first.
TEST(AllocationOrderTest, TheCityActuallyRunsIntoTheWalkerCap)
{
    const auto summary = runCity(cappedCityIn(inOrder()), kCappedTicks);

    EXPECT_EQ(summary.walkers.size(), antwika::game::kWalkerLimit)
        << "every building that asked got a walker, so nothing was "
           "contended";
}

TEST(AllocationOrderTest, TheSameCappedCityBackwardsSendsTheSameWalkers)
{
    const auto forwards =
        runCity(cappedCityIn(inOrder()), kCappedTicks);
    const auto backwards =
        runCity(cappedCityIn(reversed()), kCappedTicks);

    EXPECT_EQ(sorted(backwards).walkers, sorted(forwards).walkers);
}

TEST(AllocationOrderTest, TheSameCappedCityShuffledSendsTheSameWalkers)
{
    const auto forwards =
        runCity(cappedCityIn(inOrder()), kCappedTicks);
    const auto mixed = runCity(cappedCityIn(shuffled()), kCappedTicks);

    EXPECT_EQ(sorted(mixed).walkers, sorted(forwards).walkers);
}
