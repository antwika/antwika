#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/CityRatings.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class RatingsSystem final : public ISystem
    {
    public:
        explicit RatingsSystem(CityRatings &ratings) noexcept;

        RatingsSystem(const RatingsSystem &) = delete;
        RatingsSystem(RatingsSystem &&) = delete;

        RatingsSystem &operator=(const RatingsSystem &) = delete;
        RatingsSystem &operator=(RatingsSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        CityRatings &ratings;
    };

}
