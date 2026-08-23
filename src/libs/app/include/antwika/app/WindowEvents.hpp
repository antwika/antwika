#pragma once

#include <optional>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowId.hpp>

namespace antwika::app
{

    using antwika::gfx::IGfxBackend;
    using antwika::gfx::Size;
    using antwika::gfx::WindowId;

    struct WindowChanges final
    {
        bool closeRequested = false;

        std::optional<Size> resizedSize;
    };

    [[nodiscard]] WindowChanges windowChanges(
        IGfxBackend &backend, WindowId window);

    [[nodiscard]] bool closeRequestedOn(
        IGfxBackend &backend, WindowId window);

}
