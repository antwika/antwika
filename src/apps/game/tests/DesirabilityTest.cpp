#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/DesirabilitySystem.hpp"
#include "antwika/game/GridExtent.hpp"

using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::DesirabilityField;
using antwika::game::desirabilityAt;
using antwika::game::desirabilityFieldOf;
using antwika::game::desirabilityFrom;
using antwika::game::desirabilityOf;
using antwika::game::DesirabilitySource;
using antwika::game::DesirabilitySystem;
using antwika::game::GridExtent;
using antwika::game::kBuildingKindCount;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr GridExtent kExtent{.width = 24, .height = 24};

    struct Placement final
    {
        Cell at;
        BuildingKind kind;
    };

    [[nodiscard]] DesirabilityField fieldOf(
        World &world, const std::vector<Placement> &city)
    {
        for (const auto &placement : city)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, placement.at);
            world.add<Building>(entity, Building{.kind = placement.kind});
        }

        world.commit();

        return desirabilityFieldOf(world, kExtent);
    }
}

TEST(DesirabilityTest, OperatorEquals_SourceEqualityComparesEveryField)
{
    constexpr DesirabilitySource base{
        .contribution = 2, .radius = 5};

    const auto twin = base;
    EXPECT_EQ(base, twin);
    EXPECT_NE(
        base, (DesirabilitySource{.contribution = 3, .radius = 5}));
    EXPECT_NE(
        base, (DesirabilitySource{.contribution = 2, .radius = 6}));
}

TEST(DesirabilityTest, DesirabilityOf_NamesEveryKindWithoutFallingOff)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto source = desirabilityOf(
            static_cast<BuildingKind>(index));

        EXPECT_GE(source.radius, 0);
    }

    EXPECT_EQ(
        desirabilityOf(BuildingKind::House), (DesirabilitySource{}));

    EXPECT_EQ(
        desirabilityOf(static_cast<BuildingKind>(kBuildingKindCount)),
        desirabilityOf(BuildingKind::House));
}

TEST(DesirabilityTest, DesirabilityFrom_FallsOffLinearlyToTheRadius)
{
    constexpr DesirabilitySource source{
        .contribution = 4, .radius = 4};

    EXPECT_EQ(desirabilityFrom(source, 0), 4);
    EXPECT_EQ(desirabilityFrom(source, 1), 3);
    EXPECT_EQ(desirabilityFrom(source, 2), 2);
    EXPECT_EQ(desirabilityFrom(source, 3), 1);
    EXPECT_EQ(desirabilityFrom(source, 4), 0);
}

TEST(DesirabilityTest, DesirabilityFrom_FallsOffTheSameWayWhenNegative)
{
    constexpr DesirabilitySource source{
        .contribution = -4, .radius = 4};

    EXPECT_EQ(desirabilityFrom(source, 0), -4);
    EXPECT_EQ(desirabilityFrom(source, 2), -2);
    EXPECT_EQ(desirabilityFrom(source, 4), 0);
}

TEST(DesirabilityTest, DesirabilityFieldOf_IsEmptyWithNothingBuilt)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    EXPECT_TRUE(desirabilityFieldOf(world, kExtent).empty());
}

TEST(DesirabilityTest, DesirabilityFieldOf_SkipsAKindThatReachesNowhere)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    const auto field =
        fieldOf(world, {{.at = Cell{.x = 5, .y = 5},
                         .kind = BuildingKind::House}});

    EXPECT_TRUE(field.empty());
}

TEST(DesirabilityTest, DesirabilityFieldOf_PutsAWholeContributionOnItself)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    const auto field =
        fieldOf(world, {{.at = Cell{.x = 8, .y = 8},
                         .kind = BuildingKind::Well}});

    const auto source = desirabilityOf(BuildingKind::Well);
    ASSERT_NE(source.contribution, 0);
    ASSERT_GT(source.radius, 0);

    EXPECT_EQ(
        desirabilityAt(field, Cell{.x = 8, .y = 8}), source.contribution);

    EXPECT_EQ(
        desirabilityAt(
            field, Cell{.x = 8 + source.radius, .y = 8}),
        0);
}

TEST(DesirabilityTest, DesirabilityFieldOf_KeepsNoCellThatComesToNothing)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    const auto field = fieldOf(
        world,
        {{.at = Cell{.x = 10, .y = 10}, .kind = BuildingKind::Well},
         {.at = Cell{.x = 10, .y = 10}, .kind = BuildingKind::Farm}});

    EXPECT_EQ(field.count(Cell{.x = 10, .y = 10}), 0U);
    EXPECT_EQ(desirabilityAt(field, Cell{.x = 10, .y = 10}), 0);

    for (const auto &[cell, value] : field)
    {
        EXPECT_NE(value, 0) << cell.x << ' ' << cell.y;
    }
}

TEST(DesirabilityTest, DesirabilityFieldOf_MeasuresFromTheWholeBlock)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    const auto field =
        fieldOf(world, {{.at = Cell{.x = 8, .y = 8},
                         .kind = BuildingKind::Storage}});

    const auto source = desirabilityOf(BuildingKind::Storage);
    ASSERT_NE(source.contribution, 0);

    for (std::int32_t x = 8; x <= 10; ++x)
    {
        EXPECT_EQ(
            desirabilityAt(field, Cell{.x = x, .y = 9}),
            source.contribution);
    }
}

TEST(DesirabilityTest, DesirabilityFieldOf_KeepsTheFieldInsideTheExtent)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};

    const auto field =
        fieldOf(world, {{.at = Cell{.x = 0, .y = 0},
                         .kind = BuildingKind::Market}});

    ASSERT_FALSE(field.empty());

    for (const auto &[cell, value] : field)
    {
        EXPECT_TRUE(kExtent.contains(cell)) << cell.x << ' ' << cell.y;
    }
}

TEST(DesirabilityTest, DesirabilityFieldOf_IsTheSameUnderEitherOrder)
{
    const std::vector<Placement> city{
        {.at = Cell{.x = 4, .y = 4}, .kind = BuildingKind::ClayPit},
        {.at = Cell{.x = 9, .y = 5}, .kind = BuildingKind::Market},
        {.at = Cell{.x = 6, .y = 9}, .kind = BuildingKind::Well},
        {.at = Cell{.x = 12, .y = 12}, .kind = BuildingKind::Workshop},
        {.at = Cell{.x = 2, .y = 14}, .kind = BuildingKind::FireStation},
    };

    std::vector<Placement> reversed(city.rbegin(), city.rend());

    ::testing::NiceMock<MockLogger> logger;
    World forwards{logger};
    World backwards{logger};

    EXPECT_EQ(fieldOf(backwards, reversed), fieldOf(forwards, city));
}

TEST(DesirabilityTest, DesirabilityAt_AnswersZeroWhereNothingReaches)
{
    const DesirabilityField field;

    EXPECT_EQ(desirabilityAt(field, Cell{.x = 3, .y = 3}), 0);
}

TEST(DesirabilityTest, Update_RebuildsTheFieldFromWhatIsStanding)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    DesirabilityField field;
    DesirabilitySystem system(field, kExtent);

    const auto market = world.create();
    world.add<Cell>(market, Cell{.x = 8, .y = 8});
    world.add<Building>(market, Building{.kind = BuildingKind::Market});
    world.commit();

    system.update(world, 0);

    EXPECT_GT(desirabilityAt(field, Cell{.x = 8, .y = 8}), 0);

    world.destroy(market);
    world.commit();

    system.update(world, 1);

    EXPECT_TRUE(field.empty());
}
