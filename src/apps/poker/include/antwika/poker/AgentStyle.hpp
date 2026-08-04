#pragma once

#include <cstddef>
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
     * @brief How many styles an agent can play in.
     *
     * Declared beside the enumeration it counts, so a table keyed by
     * style states its size rather than repeating a three.
     */
    inline constexpr std::size_t kAgentStyleCount =
        static_cast<std::size_t>(AgentStyle::Aggressive) + 1;

    /**
     * @brief Name a style as a table would describe the player.
     * @param style The style to name.
     * @return A human-readable name, e.g. "aggressive".
     */
    [[nodiscard]] std::string_view toString(AgentStyle style) noexcept;

} // namespace antwika::poker
