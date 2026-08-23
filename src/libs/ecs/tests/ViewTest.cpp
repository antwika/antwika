#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "antwika/ecs/View.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

using antwika::ecs::ComponentStorage;
using antwika::ecs::Entity;
using antwika::ecs::View;

namespace
{

    struct Position final
    {
        int x{};
    };

    struct Velocity final
    {
        int dx{};
    };

}

TEST(ViewTest, View_ListsEveryEntityWithOneComponent)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});

    const View<Position> view(&positions);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}, Entity{2}}));
}

TEST(ViewTest, View_IsEmptyForANullStorage)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});

    const View<Position, Velocity> view(&positions, nullptr);

    EXPECT_EQ(view.getSize(), 0U);
}

TEST(ViewTest, View_IntersectsTwoComponents)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocitiesVelocity;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});
    positions.insert(Entity{3}, Position{});
    velocitiesVelocity.insert(Entity{2}, Velocity{});
    velocitiesVelocity.insert(Entity{3}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{2}, Entity{3}}));
}

TEST(ViewTest, View_ExcludesAnEntityAbsentElsewhere)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{5}, Position{});

    ComponentStorage<Velocity> velocitiesVelocity;
    velocitiesVelocity.insert(Entity{1}, Velocity{});
    velocitiesVelocity.insert(Entity{2}, Velocity{});
    velocitiesVelocity.insert(Entity{3}, Velocity{});
    velocitiesVelocity.insert(Entity{4}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}}));
}

TEST(ViewTest, View_ExcludesAMissingEarlierParameter)
{
    ComponentStorage<Position> positions;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});
    positions.insert(Entity{3}, Position{});
    positions.insert(Entity{4}, Position{});

    ComponentStorage<Velocity> velocitiesVelocity;
    velocitiesVelocity.insert(Entity{1}, Velocity{});
    velocitiesVelocity.insert(Entity{9}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}}));
}

TEST(ViewTest, View_TakesItsOrderFromTheFirstStorageOnASizeTie)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocitiesVelocity;
    positions.insert(Entity{1}, Position{});
    positions.insert(Entity{2}, Position{});
    velocitiesVelocity.insert(Entity{2}, Velocity{});
    velocitiesVelocity.insert(Entity{1}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{1}, Entity{2}}));
}

TEST(ViewTest, View_OrdersBySmallerStorageEitherWay)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocitiesVelocity;
    for (std::uint64_t value = 1; value <= 10; ++value)
    {
        positions.insert(Entity{value}, Position{});
    }
    velocitiesVelocity.insert(Entity{7}, Velocity{});
    velocitiesVelocity.insert(Entity{3}, Velocity{});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{Entity{7}, Entity{3}}));
}

TEST(ViewTest, Iterator_SkipsAnEntityMissingFromTheOtherStorage)
{
    ComponentStorage<Position> positions;
    ComponentStorage<Velocity> velocitiesVelocity;
    for (std::uint64_t value = 1; value <= 4; ++value)
    {
        positions.insert(Entity{value}, Position{});
    }
    velocitiesVelocity.insert(Entity{1}, Velocity{});
    velocitiesVelocity.insert(Entity{2}, Velocity{});
    velocitiesVelocity.insert(Entity{3}, Velocity{});
    velocitiesVelocity.insert(Entity{4}, Velocity{});
    velocitiesVelocity.removeAll(std::vector<Entity>{Entity{2}});

    const View<Position, Velocity> view(&positions, &velocitiesVelocity);
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(
        entities, (std::vector<Entity>{Entity{1}, Entity{3}, Entity{4}}));
}
