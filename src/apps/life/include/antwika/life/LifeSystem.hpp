#pragma once

#include <cstdint>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class LifeSystem final : public ISystem
    {
    public:
        explicit LifeSystem(const Grid &grid);

        LifeSystem(const LifeSystem &) = delete;
        LifeSystem(LifeSystem &&) = delete;

        LifeSystem &operator=(const LifeSystem &) = delete;
        LifeSystem &operator=(LifeSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        [[nodiscard]] int countAliveNeighbors(
            const World &world, std::uint32_t x, std::uint32_t y) const;

        const Grid &grid;
    };

}
