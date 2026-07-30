#pragma once

#include <cstdint>

namespace antwika::game
{

    /**
     * @brief A thing a building stocks and a walker carries.
     */
    enum class Resource : std::uint8_t
    {
        Food,
        Water,
    };

} // namespace antwika::game
