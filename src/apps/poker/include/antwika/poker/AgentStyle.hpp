#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::poker
{

    /**
     * @brief How willing a PolicyAgent is to put chips in.
     */
    enum class AgentStyle : std::uint8_t
    {
        /**
         * @brief Folds anything but a strong hand, raises only the best.
         */
        Tight = 0,

        /**
         * @brief Calls with a reasonable hand, raises a good one.
         */
        Balanced,

        /**
         * @brief Calls light and raises often.
         */
        Aggressive,
    };

    /**
     * @brief Name a style as a table would describe the player.
     * @param style The style to name.
     * @return A human-readable name, e.g. "aggressive".
     */
    [[nodiscard]] std::string_view toString(AgentStyle style) noexcept;

} // namespace antwika::poker
