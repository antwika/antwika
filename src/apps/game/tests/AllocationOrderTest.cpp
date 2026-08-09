#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

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

    constexpr antwika::time::Tick kCappedTicks = 20;

    [[nodiscard]] std::vector<SavedBuilding> cityBuildings()
    {
        return {
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

    constexpr GridExtent kWideExtent{.width = 32, .height = 32};
    constexpr std::int32_t kWellRows = 3;
    constexpr std::int32_t kWellsPerRow = 32;

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

        buildings.push_back(SavedBuilding{
            .at = Cell{.x = 0, .y = 9},
            .kind = BuildingKind::House,
            .stock = {antwika::game::kStockOnCompletion, 0, 0},
            .household = Household{
                .population = kWellRows * kWellsPerRow}});

        return buildings;
    }

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
}

TEST(AllocationOrderTest, RunCity_RunsShortOfPeople)
{
    const auto summary = runCity(cityIn({0, 1, 2, 3}));

    ASSERT_EQ(summary.buildings.size(), 4U);
    EXPECT_GT(summary.ratings.population, 0);
    EXPECT_GT(summary.ratings.employment, 0);
    EXPECT_LT(summary.ratings.employment, 100);
}

TEST(AllocationOrderTest, RunCity_IsTheSameBuiltBackwards)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));
    const auto backwards = runCity(cityIn({3, 2, 1, 0}));

    EXPECT_EQ(sorted(backwards), sorted(forwards));
}

TEST(AllocationOrderTest, RunCity_IsTheSameBuiltShuffled)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));
    const auto shuffled = runCity(cityIn({2, 0, 3, 1}));

    EXPECT_EQ(sorted(shuffled), sorted(forwards));
}

TEST(AllocationOrderTest, RunCity_RatesTheSameInAnyOrder)
{
    const auto forwards = runCity(cityIn({0, 1, 2, 3}));

    EXPECT_EQ(runCity(cityIn({3, 2, 1, 0})).ratings, forwards.ratings);
    EXPECT_EQ(runCity(cityIn({1, 3, 0, 2})).ratings, forwards.ratings);
}

TEST(AllocationOrderTest, RunCity_RunsIntoTheWalkerCap)
{
    const auto summary = runCity(cappedCityIn(inOrder()), kCappedTicks);

    EXPECT_EQ(summary.walkers.size(), antwika::game::kWalkerLimit);
}

TEST(AllocationOrderTest, RunCity_SendsTheSameWalkersBackwards)
{
    const auto forwards =
        runCity(cappedCityIn(inOrder()), kCappedTicks);
    const auto backwards =
        runCity(cappedCityIn(reversed()), kCappedTicks);

    EXPECT_EQ(sorted(backwards).walkers, sorted(forwards).walkers);
}

TEST(AllocationOrderTest, RunCity_SendsTheSameWalkersShuffled)
{
    const auto forwards =
        runCity(cappedCityIn(inOrder()), kCappedTicks);
    const auto mixed = runCity(cappedCityIn(shuffled()), kCappedTicks);

    EXPECT_EQ(sorted(mixed).walkers, sorted(forwards).walkers);
}
