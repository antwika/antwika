#pragma once

#include <antwika/gfx/IRenderer.hpp>

#include "antwika/ui/DrawList.hpp"

namespace antwika::ui
{

    using antwika::gfx::IRenderer;

    /**
     * @brief Draw a picture through a renderer.
     *
     * The only place in this library that touches antwika::gfx, and a
     * plain translation: one command becomes one call, in order, and
     * nothing is measured, decided or skipped here.
     *
     * Deliberately does not clear and does not present.
     * A UI is drawn over whatever is already there, and whoever owns the
     * frame decides when it is finished, so neither belongs to a function
     * that only knows about the UI.
     *
     * @param renderer Receives one call per command.
     * @param commands The picture, in the order it is drawn.
     */
    void paint(IRenderer &renderer, const DrawList &commands);

} // namespace antwika::ui
