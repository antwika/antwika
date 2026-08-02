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

    // One house of people, and three farms wanting more than there are.
    // Which farm goes short is the answer this test is about.
    // And it must not depend on which of them was built first.
    [[nodiscard]] std::vector<SavedBuilding> cityBuildings()
    {
        return {
            SavedBuilding{
                .at = Cell{.x = 1, .y = 5},
                .kind = BuildingKind::House,
                .stock = {antwika::game::kStockOnCompletion, 0, 0},
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

    [[nodiscard]] GameSummary runCity(const SaveGame &start)
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
                .tick = kTicks - 2,
                .event = Event{.name = antwika::engine::events::kStop}}};
        ReplaySource source(script);

        return antwika::game::bootstrap(
            antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .maxTicks = kTicks,
                .start = start});
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
