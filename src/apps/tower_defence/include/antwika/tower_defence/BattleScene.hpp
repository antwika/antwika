#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    class BattleScene final
    {
    public:
        void draw(
            IRenderer &renderer,
            Size canvas,
            const BattleSnapshot &snapshot) const;
    };

}
