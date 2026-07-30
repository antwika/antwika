#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::holdem
{

    /**
     * @brief How far a hand has progressed.
     *
     * Each of the first four stages is a betting round; Showdown is
     * reached only when two or more players are still in the hand after
     * the river, and is where cards are compared. A hand that ends by
     * everyone but one player folding never reaches it, and stops on
     * whichever stage the last fold happened on.
     */
    enum class Stage : std::uint8_t
    {
        PreFlop = 0,
        Flop,
        Turn,
        River,
        Showdown,
    };

    /**
     * @brief Name a stage as a player would.
     * @param stage The stage to name.
     * @return A human-readable name, e.g. "pre-flop".
     */
    [[nodiscard]] std::string_view toString(Stage stage) noexcept;

} // namespace antwika::holdem
