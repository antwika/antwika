#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class WalkerSystem final : public ISystem
    {
    public:
        WalkerSystem(
            const PathIndex &paths,
            const BuildingIndex &built,
            GridExtent extent);

        WalkerSystem(const WalkerSystem &) = delete;
        WalkerSystem(WalkerSystem &&) = delete;

        WalkerSystem &operator=(const WalkerSystem &) = delete;
        WalkerSystem &operator=(WalkerSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void travel(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at);

        void roam(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at,
            antwika::time::Tick tick);

        void headHome(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at);

        void runErrand(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at,
            antwika::ecs::Entity bound);

        void respond(
            World &world,
            antwika::ecs::Entity entity,
            const Walker &walker,
            Cell at);

        const PathIndex &paths;
        const BuildingIndex &built;
        GridExtent extent;
    };

}
