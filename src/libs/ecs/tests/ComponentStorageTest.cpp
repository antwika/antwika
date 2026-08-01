#include "antwika/ecs/ComponentStorage.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::makeEntity;

namespace
{

    struct Position
    {
        int x{};
        int y{};

        bool operator==(const Position &) const = default;
    };

} // namespace

TEST(ComponentStorageTest, InsertedEntityIsContainedAndReadable)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};

    storage.insert(entity, Position{1, 2});

    EXPECT_TRUE(storage.contains(entity));
    EXPECT_EQ(storage.read(entity), (Position{1, 2}));
}

TEST(ComponentStorageTest, UnknownEntityIsNotContained)
{
    ComponentStorage<Position> storage;

    EXPECT_FALSE(storage.contains(Entity{7}));
}

TEST(ComponentStorageTest, ReadingAnUnknownEntityThrows)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(
        static_cast<void>(storage.read(Entity{7})), EcsError);
}

TEST(ComponentStorageTest, WritingAnUnknownEntityThrows)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(storage.write(Entity{7}, Position{}), EcsError);
}

TEST(ComponentStorageTest, RemovingAnUnknownEntityThrows)
{
    ComponentStorage<Position> storage;

    EXPECT_THROW(storage.remove(Entity{7}), EcsError);
}

TEST(ComponentStorageTest, RemovedEntityIsNoLongerContained)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.remove(entity);

    EXPECT_FALSE(storage.contains(entity));
    EXPECT_THROW(static_cast<void>(storage.read(entity)), EcsError);
}

TEST(ComponentStorageTest, InsertOverwritesAnExistingEntity)
{
    ComponentStorage<Position> storage;
    const Entity entity{1};
    storage.insert(entity, Position{1, 2});

    storage.insert(entity, Position{9, 9});

    EXPECT_EQ(storage.read(entity), (Position{9, 9}));
}

TEST(ComponentStorageTest, AnEarlierGenerationOfAReusedIndexIsNotContained)
{
    ComponentStorage<Position> storage;
    const auto before = makeEntity(1, 0);
    const auto after = makeEntity(1, 1);
    storage.insert(before, Position{1, 2});

    EXPECT_FALSE(storage.contains(after));
    EXPECT_THROW(static_cast<void>(storage.read(after)), EcsError);
    EXPECT_THROW(storage.write(after, Position{}), EcsError);
    EXPECT_THROW(storage.remove(after), EcsError);
}

TEST(ComponentStorageTest, InsertingAReusedIndexReplacesThePreviousHolder)
{
    // World::retire() purges every pool before an index is freed.
    // So this is only reachable by driving a storage directly.
    ComponentStorage<Position> storage;
    const auto other = makeEntity(2, 0);
    const auto before = makeEntity(1, 0);
    const auto after = makeEntity(1, 1);
    storage.insert(before, Position{1, 2});
    storage.insert(other, Position{3, 4});

    storage.insert(after, Position{9, 9});

    EXPECT_FALSE(storage.contains(before));
    EXPECT_EQ(storage.read(after), (Position{9, 9}));

    const auto entities = storage.entities();
    const std::vector<Entity> order(entities.begin(), entities.end());

    EXPECT_EQ(order, (std::vector<Entity>{other, after}));
}

TEST(ComponentStorageTest, EntitiesOrderSurvivesInterleavedInsertAndRemove)
{
    ComponentStorage<Position> storage;
    const Entity a{1};
    const Entity b{2};
    const Entity c{3};
    const Entity d{4};

    storage.insert(a, Position{});
    storage.insert(b, Position{});
    storage.insert(c, Position{});
    storage.remove(b);
    storage.insert(d, Position{});

    const auto entities = storage.entities();
    const std::vector<Entity> order(entities.begin(), entities.end());

    EXPECT_EQ(order, (std::vector<Entity>{a, c, d}));
}

TEST(ComponentStorageTest,
     RepeatingTheSameChurnProducesTheSameOrderEveryTime)
{
    auto buildOrder = []
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
        return std::vector<Entity>(entities.begin(), entities.end());
    };

    EXPECT_EQ(buildOrder(), buildOrder());
}
