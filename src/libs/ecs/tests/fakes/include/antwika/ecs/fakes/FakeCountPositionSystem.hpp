#pragma once

#include <cstddef>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs::fakes
{

    template <typename PositionT>
    class FakeCountPositionSystem final : public ISystem
    {
    public:
        explicit FakeCountPositionSystem(
            std::vector<std::size_t> &seenCounts)
            : seenCounts(&seenCounts)
        {
        }

        void update(World &world, antwika::time::Tick) override
        {
            seenCounts->push_back(world.view<PositionT>().getSize());
        }

    private:
        std::vector<std::size_t> *seenCounts;
    };

}
