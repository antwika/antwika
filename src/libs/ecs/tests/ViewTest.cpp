#include "antwika/ecs/View.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::Entity;
using antwika::ecs::View;

namespace
{

    struct Position
    {
        int x{};
    };

    struct Velocity
    {
        int dx{};
    };

} // namespace

TEST(ViewTest, SingleComponentViewListsEveryEntityWithIt)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});

    const View<Position> view(&positions);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}, Entity{2}}));
}

TEST(ViewTest, NullStorageMeansAnEmptyView)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});

    const View<Position, Velocity> view(&positions, nullptr);

    EXPECT_EQ(view.size(), 0U);
}

TEST(ViewTest, TwoComponentViewIsTheIntersection)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocities;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});
    positions.insert(Entity{3}, Position{});
    velocities.insert(Entity{2}, Velocity{});
    velocities.insert(Entity{3}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocities);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{2}, Entity{3}}));
}

TEST(ViewTest, EntityInTheSmallestStorageButAbsentElsewhereIsExcluded)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{5}, Position{});

    ComponentStorage<Velocity> velocities;
    velocities.insert(Entity{1}, Velocity{});
    velocities.insert(Entity{2}, Velocity{});
    velocities.insert(Entity{3}, Velocity{});
    velocities.insert(Entity{4}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocities);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}}));
}

TEST(ViewTest, EntityMissingFromAnEarlierParameterIsExcluded)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});
    positions.insert(Entity{3}, Position{});
    positions.insert(Entity{4}, Position{});

    ComponentStorage<Velocity> velocities;
    velocities.insert(Entity{1}, Velocity{});
    velocities.insert(Entity{9}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocities);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}}));
}

TEST(ViewTest, OrderFollowsTheSmallerStorageRegardlessOfArgumentOrder)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocities;
    for (std::uint64_t value = 1; value <= 10; ++value)
    {
        positions.insert(Entity{value}, Position{});
    }
    velocities.insert(Entity{7}, Velocity{});
    velocities.insert(Entity{3}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocities);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{7}, Entity{3}}));
}
