#include <gtest/gtest.h>

#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::Entity;

namespace
{

    struct Health final
    {
        int value{};

        bool operator==(const Health &) const = default;
    };

}

TEST(DoubleBufferingTest, Write_IsInvisibleToReadUntilCommit)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{0});

    EXPECT_EQ(storage.read(entity), (Health{10}));
}

TEST(DoubleBufferingTest, Commit_MakesAWriteVisible)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{0});
    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{0}));
}

TEST(DoubleBufferingTest, Commit_TakesTheLastWriteByCallOrder)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{1});
    storage.write(entity, Health{2});
    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{2}));
}

TEST(DoubleBufferingTest, Commit_LeavesAnUntouchedEntityUnchanged)
{
    ComponentStorage<Health> storage;
    const Entity touched{1};
    const Entity untouched{2};
    storage.insert(touched, Health{10});
    storage.insert(untouched, Health{20});

    storage.write(touched, Health{99});
    storage.commit();

    EXPECT_EQ(storage.read(untouched), (Health{20}));
}

TEST(DoubleBufferingTest, Commit_KeepsTheComponentsAnEarlierRemoveShiftedDown)
{
    ComponentStorage<Health> storage;
    storage.insert(Entity{1}, Health{10});
    storage.insert(Entity{2}, Health{20});
    storage.insert(Entity{3}, Health{30});

    storage.remove(Entity{2});
    storage.commit();

    EXPECT_EQ(storage.read(Entity{1}), (Health{10}));
    EXPECT_EQ(storage.read(Entity{3}), (Health{30}));
}

TEST(DoubleBufferingTest, Commit_ReseedsTheBackFromTheFront)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});
    storage.write(entity, Health{1});
    storage.commit();

    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{1}));
}
