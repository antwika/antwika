#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/life/Board.hpp"

namespace antwika::life
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    class BoardScene final
    {
    public:
        void draw(
            IRenderer &renderer, Size canvas, const Board &board) const;
    };

}
