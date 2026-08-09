#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class HaulingSystem final : public ISystem
    {
    public:
        HaulingSystem(const PathIndex &paths, GridExtent extent);

        HaulingSystem(const HaulingSystem &) = delete;
        HaulingSystem(HaulingSystem &&) = delete;

        HaulingSystem &operator=(const HaulingSystem &) = delete;
        HaulingSystem &operator=(HaulingSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GridExtent extent;
    };

}
