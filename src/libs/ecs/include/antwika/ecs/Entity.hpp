#pragma once

#include <cstdint>

namespace antwika::ecs
{

    /**
     * @brief Identifies an entity: an index and a generation packed
     * into one plain integer, never reinterpreted as anything else.
     *
     * A scoped enum with no enumerators over std::uint64_t: trivially
     * copyable and comparable like a raw integer, but distinct enough
     * that it can't be mixed up with an unrelated integer by accident.
     *
     * The low kEntityIndexBits are the *index* — the slot EntityManager
     * and every ComponentStorage address the entity by. The rest are
     * the *generation*, which counts how many times that slot had been
     * retired when this handle was handed out.
     *
     * A retired index is handed out again, so what an ECS holds is
     * bounded by how many entities are alive at once rather than by how
     * many have ever existed. The generation is what makes that safe: a
     * handle kept from before a slot was retired names a generation the
     * slot no longer has, so alive() reads it as dead rather than as
     * whoever holds the slot now. **A stale handle can only ever be
     * dead, never somebody else** — which is what a caller caching a
     * handle and treating alive() as the authority relies on.
     *
     * Value 0 is reserved for kNullEntity, and index 0 is never handed
     * out, so no generation of it is ever alive either.
     */
    enum class Entity : std::uint64_t
    {
    };

    /**
     * @brief The entity value that never identifies a live entity.
     */
    inline constexpr Entity kNullEntity{0};

    /**
     * @brief How many of an Entity's low bits hold its index.
     *
     * The split is even, which buys 4 billion simultaneously live
     * entities and 4 billion reuses of any one slot. Neither half is
     * the one worth widening at the other's expense.
     */
    inline constexpr std::uint64_t kEntityIndexBits = 32;

    /**
     * @brief The highest index an Entity can carry.
     */
    inline constexpr std::uint64_t kMaxEntityIndex =
        (std::uint64_t{1} << kEntityIndexBits) - 1;

    /**
     * @brief The highest generation an Entity can carry.
     */
    inline constexpr std::uint64_t kMaxEntityGeneration =
        (std::uint64_t{1} << (64 - kEntityIndexBits)) - 1;

    /**
     * @brief Get the raw integer value backing an entity.
     * @param entity The entity to unwrap.
     * @return The underlying std::uint64_t value, index and generation
     * together.
     */
    [[nodiscard]] constexpr std::uint64_t rawValue(Entity entity) noexcept
    {
        return static_cast<std::uint64_t>(entity);
    }

    /**
     * @brief Get the slot an entity occupies.
     * @param entity The entity to unpack.
     * @return The index part, at most kMaxEntityIndex.
     */
    [[nodiscard]] constexpr std::uint64_t entityIndex(Entity entity) noexcept
    {
        return rawValue(entity) & kMaxEntityIndex;
    }

    /**
     * @brief Get how many times an entity's slot had been retired when
     * the entity was handed out.
     * @param entity The entity to unpack.
     * @return The generation part, at most kMaxEntityGeneration.
     */
    [[nodiscard]] constexpr std::uint64_t entityGeneration(
        Entity entity) noexcept
    {
        return rawValue(entity) >> kEntityIndexBits;
    }

    /**
     * @brief Pack an index and a generation into one entity value.
     * @param index The slot, at most kMaxEntityIndex.
     * @param generation The generation, at most kMaxEntityGeneration.
     * @return The Entity naming that generation of that slot.
     */
    [[nodiscard]] constexpr Entity makeEntity(
        std::uint64_t index, std::uint64_t generation) noexcept
    {
        return Entity{(generation << kEntityIndexBits) | index};
    }

} // namespace antwika::ecs
