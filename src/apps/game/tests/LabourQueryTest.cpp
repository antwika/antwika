#include "antwika/game/LabourQuery.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/Workforce.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::setStaff;
using antwika::game::Staff;
using antwika::game::StaffEntry;
    using antwika::game::Staffing;
    using antwika::game::staffingOf;
    using antwika::game::workedPeriod;
        using antwika::game::workersAt;
    using antwika::game::workersWantedBy;
    using antwika::log::mocks::MockLogger;

    class Scene
    {
    public:
        [[nodiscard]] Entity put(BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, Cell{});
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

// The rule the whole seam is written under.
// A workplace nothing has allocated to behaves as it always did.
TEST(LabourQueryTest, StaffingOf_ReadsAWorkplaceWithNoComponentAsFull)
{
    Scene scene;
    const auto well = scene.put(BuildingKind::Well);

    EXPECT_EQ(
        staffingOf(scene.world, well),
        (Staffing{
            .filled = workersWantedBy(BuildingKind::Well),
            .wanted = workersWantedBy(BuildingKind::Well)}));
}

TEST(LabourQueryTest, StaffingOf_ReadsTheComponentWhereThereIsOne)
{
    Scene scene;
    const auto farm = scene.put(BuildingKind::Farm);

    Staff one;
    one.sources[0] = StaffEntry{.house = farm, .count = 1};
    setStaff(scene.world, farm, one);
    scene.world.commit();

    EXPECT_EQ(
        staffingOf(scene.world, farm),
        (Staffing{
            .filled = 1,
            .wanted = workersWantedBy(BuildingKind::Farm)}));
}

// A house wants nobody, so it is fully staffed by nobody.
TEST(LabourQueryTest, StaffingOf_WantsNobodyForAKindThatEmploysNobody)
{
    Scene scene;
    const auto house = scene.put(BuildingKind::House);

    EXPECT_EQ(
        staffingOf(scene.world, house),
        (Staffing{.filled = 0, .wanted = 0}));
}

// A road, a walker, or a handle whose entity never had a Building.
TEST(LabourQueryTest, StaffingOf_AnswersForAnEntityThatIsNoBuilding)
{
    Scene scene;
    const auto road = scene.world.create();
    scene.world.add<Cell>(road, Cell{});
    scene.world.commit();

    EXPECT_EQ(
        staffingOf(scene.world, road),
        (Staffing{.filled = 0, .wanted = 0}));
}

TEST(LabourQueryTest, WorkersAt_IsTheStaffingsNumerator)
{
    Scene scene;
    const auto market = scene.put(BuildingKind::Market);

    Staff two;
    two.sources[0] = StaffEntry{.house = market, .count = 2};
    setStaff(scene.world, market, two);
    scene.world.commit();

    EXPECT_EQ(workersAt(scene.world, market), 2);
}

// setStaff() is the one writer, and it has to do both halves.
TEST(LabourQueryTest, SetStaff_AddsThenSetsTheSameComponent)
{
    Scene scene;
    const auto well = scene.put(BuildingKind::Well);

    Staff one;
    one.sources[0] = StaffEntry{.house = well, .count = 1};
    setStaff(scene.world, well, one);
    scene.world.commit();
    EXPECT_EQ(workersAt(scene.world, well), 1);

    setStaff(scene.world, well, Staff{});
    scene.world.commit();
    EXPECT_EQ(workersAt(scene.world, well), 0);
}

TEST(LabourQueryTest, WorkedPeriod_LeavesAFullComplementAlone)
{
    EXPECT_EQ(
        workedPeriod(10, Staffing{.filled = 4, .wanted = 4}), 10);
}

TEST(LabourQueryTest, WorkedPeriod_LeavesAKindThatWantsNobodyAlone)
{
    EXPECT_EQ(
        workedPeriod(10, Staffing{.filled = 0, .wanted = 0}), 10);
}

// Half the people, twice as long.
// Integer arithmetic throughout.
TEST(LabourQueryTest, WorkedPeriod_StretchesOverHoweverFewTurnedUp)
{
    EXPECT_EQ(workedPeriod(10, Staffing{.filled = 2, .wanted = 4}), 20);
    EXPECT_EQ(workedPeriod(10, Staffing{.filled = 1, .wanted = 4}), 40);
    EXPECT_EQ(workedPeriod(10, Staffing{.filled = 3, .wanted = 4}), 13);
}

// Nothing rather than a very large number.
// So a caller has to say what it does about it -- see the header.
TEST(LabourQueryTest, WorkedPeriod_AnswersNothingWhenNobodyWorksThere)
{
    EXPECT_FALSE(
        workedPeriod(10, Staffing{.filled = 0, .wanted = 4}).has_value());
}
