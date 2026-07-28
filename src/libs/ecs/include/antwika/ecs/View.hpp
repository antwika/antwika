#pragma once

#include <cstddef>
#include <limits>
#include <vector>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs
{

    /**
     * @brief Read-only snapshot of every entity that currently has every
     * component type in Ts....
     *
     * Computed once, at construction, over each storage's front buffer —
     * always safe to iterate for the lifetime of the phase it was taken
     * in, since structural changes are deferred to World::commit().
     * Order matches whichever storage has the fewest entities, which is
     * itself insertion-order-stable (see ComponentStorage), so the same
     * entity/component history always produces the same iteration order.
     */
    template <Component... Ts>
    class View final
    {
    public:
        /**
         * @brief Build the view from each component type's storage.
         * @param storages Pointer to each Ts's storage, or nullptr if no
         * entity has ever had that component type — the view is empty in
         * that case.
         */
        explicit View(const ComponentStorage<Ts> *...storages)
        {
            if ((... || (storages == nullptr)))
            {
                return;
            }

            matching = smallestEntitiesOf(storages...);
            std::erase_if(
                matching,
                [&](Entity entity)
                {
                    return !(
                        ... && storages->contains(entity)); // GCOVR_EXCL_LINE
                });
        } // GCOVR_EXCL_LINE

        using const_iterator = std::vector<Entity>::const_iterator;

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return matching.begin();
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return matching.end();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return matching.size();
        }

    private:
        template <typename... Storages>
        [[nodiscard]] static std::vector<Entity> smallestEntitiesOf(
            Storages *...storages)
        {
            std::vector<Entity> smallest;
            std::size_t smallestSize = std::numeric_limits<std::size_t>::max();

            auto consider = [&](auto *storage)
            {
                const auto entities = storage->entities();
                if (entities.size() < smallestSize) // GCOVR_EXCL_LINE
                {
                    smallestSize = entities.size();
                    smallest.assign(entities.begin(), entities.end());
                }
            };
            (consider(storages), ...);

            return smallest;
        } // GCOVR_EXCL_LINE

        std::vector<Entity> matching;
    };

} // namespace antwika::ecs
