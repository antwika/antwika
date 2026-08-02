#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Shares the city's people out among the places that want them.
     *
     * **This is the increment's one genuinely contended allocation**, and
     * everything about how it is written follows from that. The workforce
     * is the sum of everybody living in the city; every workplace declares
     * how many of them it wants through workersWantedBy(); and when the
     * two do not meet, which workplace goes short is a decision somebody
     * has to be able to predict.
     *
     * **Workplaces are walked in ascending Cell of their origin.** They
     * are collected into a std::map keyed by that cell, which is already
     * ordered by Cell::operator<=>, and **no tie-break is needed at all**
     * because BuildingIndex refuses a second building on an occupied cell
     * -- so two workplaces cannot share an origin. That is the strongest
     * form a total order comes in: one whose key is unique by
     * construction rather than by a rule stated beside it.
     *
     * **It must not read ecs::View directly, and that is the whole point
     * of the map.** A View iterates "whichever storage has the fewest
     * entities", which is reproducible for a given history but is not an
     * order anybody can name, and which *changes* as component counts
     * cross each other. It is fine for a loop whose body is independent
     * per entity. It is not fine for splitting a limited amount, which is
     * exactly what this does. AllocationOrderTest is what would catch a
     * regression here, by building one city twice in two creation orders.
     *
     * **Nothing here is a persisted event**, and this is the workstream
     * where one was most tempting. An allocation follows from the people,
     * who follow from the houses, which follow from the clicks that
     * placed them; recording it would allocate twice on replay. There is
     * no wage to set either -- a wage would be a click on a widget
     * resolved by a sink inside the tick path, which is precisely what
     * "no ui.* event name may ever exist" means.
     */
    class LabourSystem final : public ISystem
    {
    public:
        LabourSystem() = default;

        LabourSystem(const LabourSystem &) = delete;
        LabourSystem(LabourSystem &&) = delete;

        LabourSystem &operator=(const LabourSystem &) = delete;
        LabourSystem &operator=(LabourSystem &&) = delete;

        /**
         * @brief Count the city's people, then share them out.
         *
         * The count reads ecs::View directly, because a sum of integers
         * is **commutative** and no order over it is observable. The
         * share-out does not, for the reason the class comment gives.
         *
         * @param world Read for the households and the workplaces,
         * staged into with each workplace's share.
         * @param tick The tick being processed; unused, because an
         * allocation is a function of who lives in the city and of
         * nothing else.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::game
