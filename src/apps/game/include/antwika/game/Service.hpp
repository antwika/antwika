#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::game
{

    /**
     * @brief Something a walker confers on what it passes, rather than
     * hands over.
     *
     * **A service is not a Resource, and the difference is the whole
     * point of the distinction.** A good is an amount: it moves from one
     * building to another, and what one gains the other loses. A service
     * is a state: a well's carrier walking past a house makes that house
     * watered, and the well is no poorer for it. Modelling water as a
     * good made every district need a delivery per house per drain; as a
     * service it needs a walker to keep reaching it, which is the thing
     * the road network is actually for.
     *
     * Values are contiguous from zero, so a service can index a table,
     * which is how a building carries one countdown per service without
     * naming any of them.
     *
     * Nothing in this increment reads these beyond naming them: the
     * component and the system that decay coverage are the next
     * workstream's, and this header is published first precisely so that
     * one and its consumers can be written against a fixed enumeration.
     */
    enum class Service : std::uint8_t
    {
        Water = 0,   ///< Conferred by a well's water carrier.
        Health,      ///< Conferred by a doctor.
        Safety,      ///< Conferred by a fireman.
        Structure,   ///< Conferred by an engineer.
    };

    /**
     * @brief How many services there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kServiceCount =
        static_cast<std::size_t>(Service::Structure) + 1;

    /**
     * @brief Get a service's index, for addressing a per-service table.
     * @param service The service to index.
     * @return The index, always below kServiceCount.
     */
    [[nodiscard]] constexpr std::size_t serviceIndex(
        Service service) noexcept
    {
        return static_cast<std::size_t>(service);
    }

    /**
     * @brief Every service, in the enumeration's own order.
     *
     * What anything wanting one answer per service iterates, so a fifth
     * service is an enumerator here and nowhere else.
     * The static_assert below is what keeps it from drifting from the
     * enumeration it lists.
     */
    inline constexpr std::array<Service, kServiceCount> kServices{
        Service::Water,
        Service::Health,
        Service::Safety,
        Service::Structure};

    /**
     * @brief Get a service's name.
     *
     * Symbolic for resourceName()'s reason: a caption beside a coverage
     * gauge has to say which service the gauge counts, and a call site
     * naming it itself is a place a rename can miss.
     *
     * @param service The service to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view serviceName(
        Service service) noexcept
    {
        constexpr std::array<std::string_view, kServiceCount> names{
            "water",
            "health",
            "safety",
            "structure"};

        return names[serviceIndex(service) % kServiceCount];
    }

    // Indexing kServices by a service must hand that service back.
    // Otherwise a table walked in this order is a table read wrongly.
    // Resource.hpp states the same rule about its own list.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kServiceCount; ++index)
            {
                if (serviceIndex(kServices[index]) != index)
                {
                    return false;
                }
            }

            return true;
        }(),
        "kServices must list every service in its own index order");

    static_assert(serviceName(Service::Water) == "water");
    static_assert(serviceName(Service::Structure) == "structure");

} // namespace antwika::game
