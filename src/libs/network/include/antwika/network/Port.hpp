#pragma once

#include <cstdint>

namespace antwika::network
{

    /**
     * @brief The port half of an endpoint.
     *
     * A scoped enumeration rather than a bare integer for the reason
     * scheduler::Priority is one: a port and a peer count are both
     * numbers, and only one of them may be passed where the other
     * belongs.
     */
    enum class Port : std::uint16_t
    {
    };

    /**
     * @brief Get the number behind a port.
     * @param port The port to read.
     * @return Its raw value.
     */
    [[nodiscard]] constexpr std::uint16_t rawValue(Port port) noexcept
    {
        return static_cast<std::uint16_t>(port);
    }

} // namespace antwika::network
