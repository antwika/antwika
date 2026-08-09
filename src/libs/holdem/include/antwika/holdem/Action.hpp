#pragma once

#include "antwika/holdem/ActionType.hpp"
#include "antwika/holdem/Chips.hpp"

namespace antwika::holdem
{

    struct Action final
    {
        ActionType type{};

        Chips amount{};

        bool operator==(const Action &other) const = default;
    };

    [[nodiscard]] constexpr Action fold() noexcept
    {
        return Action{.type = ActionType::Fold, .amount = 0};
    }

    [[nodiscard]] constexpr Action check() noexcept
    {
        return Action{.type = ActionType::Check, .amount = 0};
    }

    [[nodiscard]] constexpr Action call() noexcept
    {
        return Action{.type = ActionType::Call, .amount = 0};
    }

    [[nodiscard]] constexpr Action bet(Chips amount) noexcept
    {
        return Action{.type = ActionType::Bet, .amount = amount};
    }

    [[nodiscard]] constexpr Action raiseTo(Chips amount) noexcept
    {
        return Action{.type = ActionType::Raise, .amount = amount};
    }

}
