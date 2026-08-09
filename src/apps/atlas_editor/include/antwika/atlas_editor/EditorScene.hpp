#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>

#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;

    class EditorScene final
    {
    public:
        void draw(
            IRenderer &renderer,
            const SceneSnapshot &snapshot,
            const ITexture *image) const;
    };

}
