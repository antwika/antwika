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
    EXPECT_EQ(storage.read(entity), (Position{1, 2}));
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
        static_cast<void>(storage.read(Entity{7})), EcsError);
}

TEST(ComponentStorageTest, Write_ThrowsOnAnUnknownEntity)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(storage.write(Entity{7}, Position{}), EcsError);
}

TEST(ComponentStorageTest, Remove_ThrowsOnAnUnknownEntity)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(storage.remove(Entity{7}), EcsError);
}

TEST(ComponentStorageTest, Remove_LeavesAnEntityNoLongerContained)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.remove(entity);

    EXPECT_FALSE(storage.contains(entity));
    EXPECT_THROW(static_cast<void>(storage.read(entity)), EcsError);
}

TEST(ComponentStorageTest, Insert_OverwritesAnExistingEntity)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.insert(entity, Position{9, 9});

    EXPECT_EQ(storage.read(entity), (Position{9, 9}));
}

TEST(ComponentStorageTest, Entities_KeepOrderAcrossInsertAndRemove)
{
    ComponentStorage<Position> storage;
    const Entity a{1};
    const Entity b{2};
    const Entity c{3};
    const Entity d{4};
    const Entity e{5};

    storage.insert(a, Position{});
    storage.insert(b, Position{});
    storage.insert(c, Position{});
    storage.insert(d, Position{});
    storage.remove(b);
    storage.insert(e, Position{});

    const auto entities = storage.entities();
    const std::vector<Entity> order(entities.begin(), entities.end());

    EXPECT_EQ(order, (std::vector<Entity>{a, c, d, e}));
}

TEST(ComponentStorageTest, Remove_MovesEachRemainingComponentWithItsEntity)
{
    ComponentStorage<Position> storage;
    storage.insert(Entity{1}, Position{10, 11});
    storage.insert(Entity{2}, Position{20, 21});
    storage.insert(Entity{3}, Position{30, 31});
    storage.insert(Entity{4}, Position{40, 41});

    storage.remove(Entity{2});

    EXPECT_EQ(storage.read(Entity{1}), (Position{10, 11}));
    EXPECT_EQ(storage.read(Entity{3}), (Position{30, 31}));
    EXPECT_EQ(storage.read(Entity{4}), (Position{40, 41}));
}

TEST(ComponentStorageTest, Remove_ShiftsRatherThanSwappingWithTheLast)
{
    ComponentStorage<Position> storage;
    for (std::uint64_t value = 1; value <= 5; ++value)
    {
        storage.insert(Entity{value}, Position{});
    }

    storage.remove(Entity{2});
    storage.remove(Entity{4});
    storage.insert(Entity{6}, Position{});

    const auto entities = storage.entities();
    const std::vector<Entity> order(entities.begin(), entities.end());

    EXPECT_EQ(
        order,
        (std::vector<Entity>{Entity{1}, Entity{3}, Entity{5}, Entity{6}}));
}
