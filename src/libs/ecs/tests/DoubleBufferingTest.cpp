#include "antwika/ecs/ComponentStorage.hpp"

#include <gtest/gtest.h>

#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::Entity;

namespace
{

    struct Health
    {
        int value{};

        bool operator==(const Health &) const = default;
    };

} // namespace

TEST(DoubleBufferingTest, WriteIsInvisibleToReadUntilCommit)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{0});

    EXPECT_EQ(storage.read(entity), (Health{10}));
}

TEST(DoubleBufferingTest, WriteBecomesVisibleAfterCommit)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{0});
    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{0}));
}

TEST(DoubleBufferingTest, LastWriteBeforeCommitWinsByCallOrder)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});

    storage.write(entity, Health{1});
    storage.write(entity, Health{2});
    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{2}));
}

TEST(DoubleBufferingTest, UntouchedEntitySurvivesACommitUnchanged)
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

TEST(DoubleBufferingTest, BackIsReseededFromFrontAfterCommit)
{
    ComponentStorage<Health> storage;
    const Entity entity{1};
    storage.insert(entity, Health{10});
    storage.write(entity, Health{1});
    storage.commit();

    // Without a further write(), a second commit must not revert to a
    // stale back value from before the first commit.
    storage.commit();

    EXPECT_EQ(storage.read(entity), (Health{1}));
}
