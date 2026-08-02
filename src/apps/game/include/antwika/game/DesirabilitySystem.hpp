#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Keeps the desirability field in step with what is standing.
     *
     * **It rebuilds the whole field rather than editing it.** A field
     * kept up to date by adding a building's contribution when it goes
     * up and subtracting it when it comes down is a second truth: it
     * would have to be told about a demolition by risk, a demolition by
     * starvation, a city switch and a save restore, and every one of
     * those it was not told about would be a district that stayed nice
     * because something that burned down was still counted. Rebuilding
     * from the buildings makes the field a pure function of them, which
     * is also what makes it identical under any creation order -- see
     * desirabilityFieldOf().
     *
     * The field lives outside the World, beside the path and building
     * indices, because it is a lookup over the whole grid rather than a
     * fact about any one entity, and because the rules that read it --
     * housing, and the city's ratings -- are not systems that own it.
     */
    class DesirabilitySystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the field it keeps.
         * @param field Overwritten every tick; must outlive this system.
         * @param extent The bounds the field is kept inside.
         */
        DesirabilitySystem(
            DesirabilityField &field, GridExtent extent) noexcept;

        DesirabilitySystem(const DesirabilitySystem &) = delete;
        DesirabilitySystem(DesirabilitySystem &&) = delete;

        DesirabilitySystem &operator=(const DesirabilitySystem &) = delete;
        DesirabilitySystem &operator=(DesirabilitySystem &&) = delete;

        /**
         * @brief Rebuild the field from what is standing right now.
         * @param world Read for the buildings; nothing is staged into it.
         * @param tick The tick being processed; unused, because the
         * field is a function of the buildings and of nothing else.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        DesirabilityField &field;
        GridExtent extent;
    };

} // namespace antwika::game
