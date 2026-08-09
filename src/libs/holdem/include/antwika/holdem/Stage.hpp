#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::holdem
{

    enum class Stage : std::uint8_t
    {
        PreFlop = 0,
        Flop,
        Turn,
        River,
        Showdown,
    };

    [[nodiscard]] constexpr Stage enumBound(Stage) noexcept
    {
        return Stage::Showdown;
    }

    [[nodiscard]] std::string_view toString(Stage stage) noexcept;

}
