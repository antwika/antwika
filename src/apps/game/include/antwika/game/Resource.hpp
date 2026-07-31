#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

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

    /**
     * @brief Every resource, in the enumeration's own order.
     *
     * What anything wanting one answer per resource iterates, so a sixth
     * good is an enumerator here and nowhere else -- the same move the
     * per-resource stock table makes.
     * The static_assert below is what keeps it from drifting from the
     * enumeration it lists.
     */
    inline constexpr std::array<Resource, kResourceCount> kResources{
        Resource::Food,
        Resource::Water};

    /**
     * @brief Get a resource's name.
     *
     * Symbolic rather than the English word written at a call site, for
     * buildingKindName()'s reason: what is drawn beside a bar has to say
     * which resource the bar counts, and two call sites naming it
     * themselves are two places a rename can miss.
     *
     * @param resource The resource to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view resourceName(
        Resource resource) noexcept
    {
        constexpr std::array<std::string_view, kResourceCount> names{
            "food",
            "water"};

        return names[resourceIndex(resource) % kResourceCount];
    }

    // Indexing kResources by a resource must hand that resource back.
    // Otherwise a table walked in this order is a table read wrongly.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kResourceCount; ++index)
            {
                if (resourceIndex(kResources[index]) != index)
                {
                    return false;
                }
            }

            return true;
        }(),
        "kResources must list every resource in its own index order");

    static_assert(resourceName(Resource::Food) == "food");
    static_assert(resourceName(Resource::Water) == "water");

} // namespace antwika::game
