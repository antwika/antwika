#pragma once

#include <variant>

#include "antwika/gfx/CloseRequested.hpp"
#include "antwika/gfx/Resized.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    using WindowEventPayload = std::variant<CloseRequested, Resized>;

    struct WindowEvent final
    {
        WindowId window = kNullWindowId;
        WindowEventPayload payload;

        [[nodiscard]] bool operator==(const WindowEvent &other) const = default;
    };

}
