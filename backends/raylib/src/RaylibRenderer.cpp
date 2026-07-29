#include "RaylibRenderer.hpp"

#include <raylib.h>

namespace antwika::gfx::raylib
{

    namespace
    {
        ::Color toRaylib(Color color)
        {
            return ::Color{
                .r = color.red,
                .g = color.green,
                .b = color.blue,
                .a = color.alpha};
        }
    } // namespace

    void RaylibRenderer::clear(Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        ClearBackground(toRaylib(color));
    }

    void RaylibRenderer::drawRect(Rect rect, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        DrawRectangle(
            rect.origin.x,
            rect.origin.y,
            static_cast<int>(rect.size.width),
            static_cast<int>(rect.size.height),
            toRaylib(color));
    }

    void RaylibRenderer::present()
    {
        if (!drawing)
        {
            return;
        }

        EndDrawing();
        drawing = false;
    }

    void RaylibRenderer::detach()
    {
        present();
        attached = false;
    }

    void RaylibRenderer::beginIfNeeded()
    {
        if (drawing || !attached)
        {
            return;
        }

        BeginDrawing();
        drawing = true;
    }

} // namespace antwika::gfx::raylib
