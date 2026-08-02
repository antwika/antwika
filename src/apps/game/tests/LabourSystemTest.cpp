#include "antwika/game/LabourSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/LabourQuery.hpp"
#include "antwika/game/Workforce.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::Household;
    using antwika::game::LabourSystem;
    using antwika::game::setHousehold;
    using antwika::game::workersAt;
    using antwika::game::workersWantedBy;
    using antwika::game::Workforce;
    using antwika::log::mocks::MockLogger;

    class Scene
    {
    public:
        Entity put(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        Entity house(Cell at, std::int32_t people)
        {
            const auto entity = put(at, BuildingKind::House);
            setHousehold(world, entity, Household{.population = people});
            world.commit();
            return entity;
        }

        void run()
        {
            system.update(world, 0);
            world.commit();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        LabourSystem system;
    };
} // namespace

TEST(LabourSystemTest, Update_StaffsEveryWorkplaceWhenThereAreEnoughPeople)
{
    Scene scene;
    scene.house(Cell{.x = 0, .y = 0}, 20);
    const auto well = scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Well);
    const auto farm = scene.put(Cell{.x = 4, .y = 0}, BuildingKind::Farm);

    scene.run();

    EXPECT_EQ(workersAt(scene.world, well), workersWantedBy(
        BuildingKind::Well));
    EXPECT_EQ(workersAt(scene.world, farm), workersWantedBy(
        BuildingKind::Farm));
}

// The whole point of the map: the lowest cell is served first.
TEST(LabourSystemTest, Update_SharesAShortWorkforceOutInAscendingCellOrder)
{
    Scene scene;
    scene.house(Cell{.x = 9, .y = 9}, 5);

    // Farms want four each, so five people cannot staff two of them.
    const auto low = scene.put(Cell{.x = 1, .y = 0}, BuildingKind::Farm);
    const auto high = scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Farm);
    const auto highest =
        scene.put(Cell{.x = 3, .y = 0}, BuildingKind::Farm);

    scene.run();

    EXPECT_EQ(workersAt(scene.world, low), 4);
    EXPECT_EQ(workersAt(scene.world, high), 1);
    EXPECT_EQ(workersAt(scene.world, highest), 0);
}

// Cell orders on x before y, so a lower column wins whatever the row.
TEST(LabourSystemTest, Update_OrdersByColumnBeforeRow)
{
    Scene scene;
    scene.house(Cell{.x = 9, .y = 9}, 1);

    const auto left = scene.put(Cell{.x = 0, .y = 8}, BuildingKind::Well);
    const auto right = scene.put(Cell{.x = 1, .y = 0}, BuildingKind::Well);

    scene.run();

    EXPECT_EQ(workersAt(scene.world, left), 1);
    EXPECT_EQ(workersAt(scene.world, right), 0);
}

// A house is somewhere people live, never somewhere they work.
TEST(LabourSystemTest, Update_GivesNoWorkforceToAKindThatWantsNobody)
{
    Scene scene;
    const auto home = scene.house(Cell{.x = 0, .y = 0}, 5);
    const auto store =
        scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Storage);

    scene.run();

    EXPECT_FALSE(scene.world.has<Workforce>(home));
    EXPECT_FALSE(scene.world.has<Workforce>(store));
}

TEST(LabourSystemTest, Update_StaffsNobodyInACityWithNoPeopleInIt)
{
    Scene scene;
    const auto well = scene.put(Cell{.x = 1, .y = 1}, BuildingKind::Well);

    scene.run();

    EXPECT_EQ(workersAt(scene.world, well), 0);
}

// People leaving takes the workers with them, on the next allocation.
TEST(LabourSystemTest, Update_TakesWorkersBackWhenThePeopleAreGone)
{
    Scene scene;
    const auto home = scene.house(Cell{.x = 0, .y = 0}, 4);
    const auto farm = scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Farm);

    scene.run();
    EXPECT_EQ(workersAt(scene.world, farm), 4);

    setHousehold(scene.world, home, Household{.population = 1});
    scene.world.commit();

    scene.run();
    EXPECT_EQ(workersAt(scene.world, farm), 1);
}

namespace
{
    // The same city, built in whichever order the caller asks for.
    // A view's order is "whichever storage has the fewest entities".
    // So building them the other way round is what would show one.
    // See LabourSystem.
    [[nodiscard]] std::vector<std::int32_t> allocatedIn(
        const std::vector<std::size_t> &order)
    {
        Scene scene;

        const std::vector<Cell> at{
            Cell{.x = 1, .y = 0},
            Cell{.x = 3, .y = 0},
            Cell{.x = 5, .y = 0},
            Cell{.x = 7, .y = 0}};

        std::vector<Entity> built(at.size());

        scene.house(Cell{.x = 0, .y = 5}, 6);

        for (const auto index : order)
        {
            built[index] = scene.put(at[index], BuildingKind::Farm);
        }

        scene.run();

        std::vector<std::int32_t> employed;

        for (const auto entity : built)
        {
            employed.push_back(workersAt(scene.world, entity));
        }

        return employed;
    }
} // namespace

TEST(LabourSystemTest, Update_AllocatesIdenticallyUnderTwoCreationOrders)
{
    const auto forwards = allocatedIn({0, 1, 2, 3});
    const auto backwards = allocatedIn({3, 2, 1, 0});
    const auto shuffled = allocatedIn({2, 0, 3, 1});

    // Six people, four farms wanting four each.
    EXPECT_EQ(forwards, (std::vector<std::int32_t>{4, 2, 0, 0}));
    EXPECT_EQ(backwards, forwards);
    EXPECT_EQ(shuffled, forwards);
}
