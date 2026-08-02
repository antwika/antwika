#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/CityRatings.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Keeps the city's ratings in step with the city.
     *
     * DesirabilitySystem's counterpart, and it is written the same way
     * and for the same reasons: it **recomputes the whole answer** rather
     * than keeping a running total that would have to be told about a
     * demolition by risk, a house losing its last occupant, a city switch
     * and a save restore -- and every one of those it was not told about
     * would be a rating that flattered a city that no longer existed.
     *
     * The value lives outside the World, beside the desirability field,
     * because it is a fact about the whole city rather than about any one
     * entity, and because what reads it -- the toolbar -- is not a system
     * that could own it.
     *
     * **It belongs in the observe phase, ahead of the observers**, which
     * is the one place it sees the tick it is reporting on rather than
     * the tick before: everything that moves a person or a job has
     * already run and committed by then.
     */
    class RatingsSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the ratings it keeps.
         * @param ratings Overwritten every tick; must outlive this
         * system.
         */
        explicit RatingsSystem(CityRatings &ratings) noexcept;

        RatingsSystem(const RatingsSystem &) = delete;
        RatingsSystem(RatingsSystem &&) = delete;

        RatingsSystem &operator=(const RatingsSystem &) = delete;
        RatingsSystem &operator=(RatingsSystem &&) = delete;

        /**
         * @brief Rate the city as it stands right now.
         * @param world Read for the buildings; nothing is staged into it.
         * @param tick The tick being processed; unused, because a rating
         * is a function of what is standing and of nothing else.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        CityRatings &ratings;
    };

} // namespace antwika::game
