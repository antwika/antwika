#pragma once

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    /**
     * @brief A fireman's orders to a burning ruin.
     *
     * **Errand's and Journey's sibling, and a component of its own for
     * their reason**: an errand is about goods and a journey is about
     * a person moving house, and a fireman answering a fire is
     * neither. A walker carries at most one of the three.
     *
     * The target is a handle rather than a cell, so a ruin razed while
     * the fireman is on his way is answered by the world rather than
     * by a stale coordinate -- the same arm Errand's destination and
     * Journey's house take.
     *
     * RuinSystem is the only writer: it tasks the nearest free fireman
     * with each fire that has nobody coming, one fireman per fire.
     * WalkerSystem steers the carrier **across open ground** rather
     * than along the roads -- a fire does not wait for paving -- and
     * on arrival puts the fire out and destroys the walker.
     */
    struct FireCall
    {
        /** @brief The burning ruin being answered. */
        antwika::ecs::Entity target = antwika::ecs::kNullEntity;

        /**
         * @brief Compare two calls.
         * @param other The call to compare against.
         * @return True when both name the same ruin.
         */
        [[nodiscard]] bool operator==(const FireCall &other) const
            = default;
    };

} // namespace antwika::game
