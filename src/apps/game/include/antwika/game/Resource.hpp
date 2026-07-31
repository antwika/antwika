#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::game
{

    /**
     * @brief A good a walker carries and a house consumes.
     *
     * Values are contiguous from zero, so a resource can index a table --
     * which is how a building holds one amount per resource without
     * naming either of them.
     */
    enum class Resource : std::uint8_t
    {
        Food = 0,
        Water,
    };

    /**
     * @brief How many resources there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kResourceCount =
        static_cast<std::size_t>(Resource::Water) + 1;

    /**
     * @brief Get a resource's index, for addressing a per-resource table.
     * @param resource The resource to index.
     * @return The index, always below kResourceCount.
     */
    [[nodiscard]] constexpr std::size_t resourceIndex(
        Resource resource) noexcept
    {
        return static_cast<std::size_t>(resource);
    }

} // namespace antwika::game
