#pragma once

#include <cstddef>
#include <limits>
#include <vector>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs
{

    template <Component... Ts>
    class View final
    {
    public:
        explicit View(const ComponentStorage<Ts> *...storages)
        {
            if ((... || (storages == nullptr)))
            {
                return;
            }

            matching = smallestEntitiesOf(storages...);
            std::erase_if(
                matching,
                [&](Entity entity) // GCOVR_EXCL_LINE
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

            auto consider = [&](auto *storage) // GCOVR_EXCL_LINE
            {
                const auto entities = storage->entities();
                if (entities.size() < smallestSize) // GCOVR_EXCL_LINE
                {
                    smallestSize = entities.size();

                    // GCOVR_EXCL_START
                    smallest.assign(entities.begin(), entities.end());
                    // GCOVR_EXCL_STOP
                }
            };
            (consider(storages), ...);

            return smallest;
        } // GCOVR_EXCL_LINE

        std::vector<Entity> matching;
    };

}
