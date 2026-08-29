#pragma once

#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>

namespace antwika::editor
{

    struct GizmoSet final
    {
        gfx::Bitmap sheetBitmap;

        std::unique_ptr<gfx::ITexture> texture;

        bool unsaved = false;
    };

}
