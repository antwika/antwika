#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class PauseGatedSystem final : public ISystem
    {
    public:
        PauseGatedSystem(ISystem &inner, const PauseState &pause) noexcept;

        PauseGatedSystem(const PauseGatedSystem &) = delete;
        PauseGatedSystem(PauseGatedSystem &&) = delete;

        PauseGatedSystem &operator=(const PauseGatedSystem &) = delete;
        PauseGatedSystem &operator=(PauseGatedSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        antwika::ecs_commons::GatedSystem gate;
    };

}
