#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::game
{

    /**
     * @brief A good a walker carries, a store holds and a house
     * consumes.
     *
     * Values are contiguous from zero, so a resource can index a table
     * -- which is how a building holds one amount per resource without
     * naming any of them.
     *
     * **Water is deliberately not here.**
     * A well confers coverage on what its carrier walks past rather than
     * handing an amount over, so water is a Service and never a number a
     * building holds; see Service.hpp.
     */
    enum class Resource : std::uint8_t
    {
        Food = 0,
        Clay,
        Pottery,
    };

    /**
     * @brief How many resources there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kResourceCount =
        static_cast<std::size_t>(Resource::Pottery) + 1;

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
     * What anything wanting one answer per resource iterates, so a
     * fourth good is an enumerator here and nowhere else -- the same
     * move the per-resource stock table makes.
     * The static_assert below is what keeps it from drifting from the
     * enumeration it lists.
     */
    inline constexpr std::array<Resource, kResourceCount> kResources{
        Resource::Food,
        Resource::Clay,
        Resource::Pottery};

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
            "clay",
            "pottery"};

        return names[resourceIndex(resource) % kResourceCount];
    }

    /**
     * @brief Check whether a house's life depends on this resource.
     *
     * **What a house runs out of is what kills it, and only food does.**
     * Before this vocabulary every resource was a survival requirement,
     * because both of them -- food and water -- were amounts a walker
     * handed over. Clay is an industrial input a house never sees, and
     * pottery decides how well a household lives rather than whether it
     * lives at all, so a table saying which is which is what keeps "runs
     * out and is lost" from meaning "is lost the moment it is built".
     *
     * A table rather than a comparison against one enumerator, for the
     * reason consumes() is one: the fourth good will be a comfort too,
     * and the negation of a single name is not a statement anybody can
     * extend.
     *
     * @param resource The resource to ask about.
     * @return True for a resource a house cannot go without.
     */
    [[nodiscard]] constexpr bool sustains(Resource resource) noexcept
    {
        constexpr std::array<bool, kResourceCount> sustaining{
            true,   // Food
            false,  // Clay
            false,  // Pottery
        };

        return sustaining[resourceIndex(resource) % kResourceCount];
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
    static_assert(resourceName(Resource::Pottery) == "pottery");
    static_assert(sustains(Resource::Food));
    static_assert(!sustains(Resource::Clay));

} // namespace antwika::game
