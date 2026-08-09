#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    enum class Action : std::uint8_t
    {
        Pause = 0,

        ZoomIn,

        ZoomOut,

        ResetView,

        ConsoleToggle,

        ConsoleExecute,
    };

    [[nodiscard]] constexpr Action enumBound(Action) noexcept
    {
        return Action::ConsoleExecute;
    }

    inline constexpr std::array<Action, 6> kActions{
        Action::Pause,
        Action::ZoomIn,
        Action::ZoomOut,
        Action::ResetView,
        Action::ConsoleToggle,
        Action::ConsoleExecute};

    inline constexpr std::size_t kActionCount = kActions.size();

    [[nodiscard]] constexpr std::size_t actionIndex(Action action) noexcept
    {
        return static_cast<std::size_t>(action);
    }

    [[nodiscard]] std::string_view actionName(Action action) noexcept;

    [[nodiscard]] std::optional<Action> actionFromName(
        std::string_view name) noexcept;

    [[nodiscard]] MessageId actionLabel(
        Action action) noexcept;

}
