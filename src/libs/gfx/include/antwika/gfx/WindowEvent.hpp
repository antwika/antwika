#pragma once

#include <variant>

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    struct CloseRequested final
    {
        [[nodiscard]] bool operator==(
            const CloseRequested &other) const = default;
    };

    struct Resized final
    {
        Size size;

        [[nodiscard]] bool operator==(const Resized &other) const = default;
    };

    using WindowEventPayload = std::variant<CloseRequested, Resized>;

    struct WindowEvent final
    {
        WindowId window = kNullWindowId;
        WindowEventPayload payload;

        [[nodiscard]] bool operator==(const WindowEvent &other) const = default;
    };

}
