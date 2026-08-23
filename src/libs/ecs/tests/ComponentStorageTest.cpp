#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::EcsError;
using antwika::ecs::Entity;

namespace
{

    struct Position final
    {
        int x{};
        int y{};

        bool operator==(const Position &) const = default;
    };

}

TEST(ComponentStorageTest, Insert_MakesAnEntityContainedAndReadable)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};

    storage.insert(entity, Position{1, 2});

    EXPECT_TRUE(storage.contains(entity));
    EXPECT_EQ(storage.getContents(entity), (Position{1, 2}));
}

TEST(ComponentStorageTest, Contains_IsFalseForAnUnknownEntity)
{
    ComponentStorage<Position> storage;

    EXPECT_FALSE(storage.contains(Entity{7}));
}

TEST(ComponentStorageTest, Read_ThrowsOnAnUnknownEntity)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(
        static_cast<void>(storage.getContents(Entity{7})), EcsError);
}

TEST(ComponentStorageTest, Write_ThrowsOnAnUnknownEntity)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(storage.write(Entity{7}, Position{}), EcsError);
}

TEST(ComponentStorageTest, RemoveAll_LeavesAnEntityNoLongerContained)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.removeAll(std::vector<Entity>{entity});

    EXPECT_FALSE(storage.contains(entity));
    EXPECT_THROW(static_cast<void>(storage.getContents(entity)), EcsError);
}

TEST(ComponentStorageTest, Insert_OverwritesAnExistingEntity)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.insert(entity, Position{9, 9});

    EXPECT_EQ(storage.getContents(entity), (Position{9, 9}));
}

TEST(ComponentStorageTest, Entities_KeepOrderAcrossInsertAndRemove)
{
    ComponentStorage<Position> storage;
    const Entity aEntity{1};
    const Entity bEntity{2};
    const Entity cEntity{3};
    const Entity dEntity{4};
    const Entity entity{5};

    storage.insert(aEntity, Position{});
    storage.insert(bEntity, Position{});
    storage.insert(cEntity, Position{});
    storage.insert(dEntity, Position{});
    storage.removeAll(std::vector<Entity>{bEntity});
    storage.insert(entity, Position{});

    const auto entities = storage.getEntities();
    const std::vector<Entity> orderEntities(entities.begin(), entities.end());

    EXPECT_EQ(
        orderEntities,
        (std::vector<Entity>{aEntity, cEntity, dEntity, entity}));
}

TEST(ComponentStorageTest, RemoveAll_MovesEachRemainingComponentWithItsEntity)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});
    storage.insert(Entity{2}, Position{20, 21});
    storage.insert(Entity{3}, Position{30, 31});
    storage.insert(Entity{4}, Position{40, 41});

    storage.removeAll(std::vector<Entity>{Entity{2}});

    EXPECT_EQ(storage.getContents(Entity{1}), (Position{10, 11}));
    EXPECT_EQ(storage.getContents(Entity{3}), (Position{30, 31}));
    EXPECT_EQ(storage.getContents(Entity{4}), (Position{40, 41}));
}

TEST(ComponentStorageTest, RemoveAll_ShiftsRatherThanSwappingWithTheLast)
{
    ComponentStorage<Position> storage;
    for (std::uint64_t value = 1; value <= 5; ++value)
    {
        storage.insert(Entity{value}, Position{});
    }

    storage.removeAll(std::vector<Entity>{Entity{2}});
    storage.removeAll(std::vector<Entity>{Entity{4}});
    storage.insert(Entity{6}, Position{});

    const auto entities = storage.getEntities();
    const std::vector<Entity> orderEntities(entities.begin(), entities.end());

    EXPECT_EQ(
        orderEntities,
        (std::vector<Entity>{Entity{1}, Entity{3}, Entity{5}, Entity{6}}));
}

TEST(ComponentStorageTest, RemoveAll_DropsEveryEntityInTheBatch)
{
    ComponentStorage<Position> storage;
    for (std::uint64_t value = 1; value <= 5; ++value)
    {
        storage.insert(Entity{value}, Position{});
    }

    const std::vector<Entity> batchEntities{Entity{2}, Entity{4}};
    storage.removeAll(batchEntities);

    EXPECT_FALSE(storage.contains(Entity{2}));
    EXPECT_FALSE(storage.contains(Entity{4}));
    EXPECT_TRUE(storage.contains(Entity{1}));
    EXPECT_TRUE(storage.contains(Entity{3}));
    EXPECT_TRUE(storage.contains(Entity{5}));
}

TEST(ComponentStorageTest, RemoveAll_KeepsTheOrderOfTheSurvivors)
{
    ComponentStorage<Position> storage;
    for (std::uint64_t value = 1; value <= 6; ++value)
    {
        storage.insert(Entity{value}, Position{});
    }

    const std::vector<Entity> batchEntities{Entity{2}, Entity{5}};
    storage.removeAll(batchEntities);

    const auto entities = storage.getEntities();
    const std::vector<Entity> orderEntities(entities.begin(), entities.end());

    EXPECT_EQ(
        orderEntities,
        (std::vector<Entity>{
            Entity{1}, Entity{3}, Entity{4}, Entity{6}}));
}

TEST(ComponentStorageTest, RemoveAll_MovesEachSurvivingComponentWithItsEntity)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});
    storage.insert(Entity{2}, Position{20, 21});
    storage.insert(Entity{3}, Position{30, 31});
    storage.insert(Entity{4}, Position{40, 41});

    const std::vector<Entity> batchEntities{Entity{1}, Entity{3}};
    storage.removeAll(batchEntities);

    EXPECT_EQ(storage.getContents(Entity{2}), (Position{20, 21}));
    EXPECT_EQ(storage.getContents(Entity{4}), (Position{40, 41}));
}

TEST(ComponentStorageTest, RemoveAll_IgnoresAnEntityItDoesNotHold)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});
    storage.insert(Entity{2}, Position{20, 21});

    const std::vector<Entity> batchEntities{Entity{9}, Entity{1}};
    storage.removeAll(batchEntities);

    EXPECT_FALSE(storage.contains(Entity{1}));
    EXPECT_EQ(storage.getContents(Entity{2}), (Position{20, 21}));
}

TEST(ComponentStorageTest, RemoveAll_LeavesTheStorageAloneWhenNothingMatches)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});
    storage.insert(Entity{2}, Position{20, 21});

    const std::vector<Entity> batchEntities{Entity{8}, Entity{9}};
    storage.removeAll(batchEntities);

    const auto entities = storage.getEntities();
    const std::vector<Entity> orderEntities(entities.begin(), entities.end());

    EXPECT_EQ(orderEntities, (std::vector<Entity>{Entity{1}, Entity{2}}));
    EXPECT_EQ(storage.getContents(Entity{1}), (Position{10, 11}));
}

TEST(ComponentStorageTest, RemoveAll_AcceptsAnEmptyBatch)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});

    storage.removeAll(std::vector<Entity>{});

    EXPECT_TRUE(storage.contains(Entity{1}));
}

TEST(ComponentStorageTest, RemoveAll_LeavesALaterInsertAddressable)
{
    ComponentStorage<Position> storage;
    for (std::uint64_t value = 1; value <= 4; ++value)
    {
        storage.insert(Entity{value}, Position{});
    }

    const std::vector<Entity> batchEntities{Entity{1}, Entity{2}};
    storage.removeAll(batchEntities);
    storage.insert(Entity{7}, Position{70, 71});

    EXPECT_EQ(storage.getContents(Entity{7}), (Position{70, 71}));
    EXPECT_EQ(storage.getContents(Entity{3}), (Position{}));
    EXPECT_TRUE(storage.contains(Entity{4}));
}
