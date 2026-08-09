#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class SessionGatedSystem final : public ISystem
    {
    public:
        SessionGatedSystem(
            ISystem &inner, const AppModeState &mode) noexcept;

        SessionGatedSystem(const SessionGatedSystem &) = delete;
        SessionGatedSystem(SessionGatedSystem &&) = delete;

        SessionGatedSystem &operator=(const SessionGatedSystem &) = delete;
        SessionGatedSystem &operator=(SessionGatedSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        antwika::ecs_commons::GatedSystem gate;
    };

}
