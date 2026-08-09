#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::holdem
{

    enum class ActionType : std::uint8_t
    {
        Fold = 0,

        Check,

        Call,

        Bet,

        Raise,
    };

    [[nodiscard]] std::string_view toString(ActionType type) noexcept;

}
