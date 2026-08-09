#pragma once

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowId.hpp>

namespace antwika::app
{

    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowId;

    [[nodiscard]] bool closeRequestedOn(
        IGfxBackend &backend, WindowId window);

}
