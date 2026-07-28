#pragma once

#include <cstdint>

namespace antwika::ecs
{

    /**
     * @brief Identifies an entity: a plain integer, never reinterpreted
     * as anything else.
     *
     * A scoped enum with no enumerators over std::uint64_t: trivially
     * copyable and comparable like a raw integer, but distinct enough
     * that it can't be mixed up with an unrelated integer by accident.
     * Value 0 is reserved for kNullEntity; EntityManager hands out 1
     * and up.
     */
    enum class Entity : std::uint64_t
    {
    };

    /**
     * @brief The entity value that never identifies a live entity.
     */
    inline constexpr Entity kNullEntity{0};

    /**
     * @brief Get the raw integer value backing an entity.
     * @param entity The entity to unwrap.
     * @return The underlying std::uint64_t value.
     */
    [[nodiscard]] constexpr std::uint64_t rawValue(Entity entity) noexcept
    {
        return static_cast<std::uint64_t>(entity);
    }

} // namespace antwika::ecs
