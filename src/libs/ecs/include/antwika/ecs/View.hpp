#pragma once

#include <cstddef>
#include <iterator>
#include <limits>
#include <span>
#include <tuple>

#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs
{

    template <Component... Ts>
    class View final
    {
    public:
        using Pools = std::tuple<const ComponentStorage<Ts> *...>;

        explicit View(const ComponentStorage<Ts> *...storages)
            : pools(storages...)
        {
            if ((... || (storages == nullptr)))
            {
                return;
            }

            drivingEntities = smallestEntitiesOf(storages...);
        } // GCOVR_EXCL_LINE

        class const_iterator final
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = Entity;
            using difference_type = std::ptrdiff_t;
            using pointer = const Entity *;
            using reference = const Entity &;

            const_iterator() = default;

            const_iterator(
                std::span<const Entity> entities,
                std::size_t slotIndex,
                Pools pools)
                : entities(entities), slotIndex(slotIndex), pools(pools)
            {
                skipToMatch();
            }

            [[nodiscard]] reference operator*() const noexcept
            {
                return entities[slotIndex];
            }

            const_iterator &operator++() noexcept
            {
                ++slotIndex;
                skipToMatch();
                return *this;
            }

            [[nodiscard]] bool operator==(
                const const_iterator &other) const noexcept
            {
                return slotIndex == other.slotIndex;
            }

        private:
            void skipToMatch() noexcept
            {
                while (
                slotIndex < entities.size() && !matches(entities[slotIndex]))
                {
                    ++slotIndex;
                }
            }

            [[nodiscard]] bool matches(Entity entity) const noexcept
            {
                return std::apply(
                    [entity](const auto *...storages) // GCOVR_EXCL_LINE
                    {
                        // GCOVR_EXCL_START
                        return (... && storages->contains(entity));
                        // GCOVR_EXCL_STOP
                    },
                    pools);
            }

            std::span<const Entity> entities{};
            std::size_t slotIndex{};
            Pools pools{};
        };

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return const_iterator(drivingEntities, 0, pools);
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return const_iterator(
                drivingEntities, drivingEntities.size(), pools);
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return static_cast<std::size_t>(std::distance(begin(), end()));
        }

    private:
        template <typename... Storages>
        [[nodiscard]] static std::span<const Entity> smallestEntitiesOf(
            Storages *...storages)
        {
            std::span<const Entity> smallestEntities;
            std::size_t smallestSize = std::numeric_limits<std::size_t>::max();

            auto consider = [&](auto *storage) // GCOVR_EXCL_LINE
            {
                const auto entities = storage->entities();
                if (entities.size() < smallestSize) // GCOVR_EXCL_LINE
                {
                    smallestSize = entities.size();
                    smallestEntities = entities;
                }
            };
            (consider(storages), ...);

            return smallestEntities;
        } // GCOVR_EXCL_LINE

        Pools pools;
        std::span<const Entity> drivingEntities{};
    };

}
