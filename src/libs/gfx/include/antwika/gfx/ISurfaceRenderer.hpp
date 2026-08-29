#pragma once

#include <string_view>

#include "antwika/gfx/ClipScope.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Glyphs.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"

namespace antwika::gfx
{

    class ISurfaceRenderer
    {
    public:
        virtual ~ISurfaceRenderer() = default;

        virtual void clear(Color color) = 0;

        virtual void drawRect(RectF rect, Color color) = 0;

        virtual void drawLine(
            PointF fromPoint, PointF toPoint, Color color) = 0;

        virtual void beginClip(RectF areaRect) = 0;

        virtual void endClip() = 0;

        virtual void drawText(
            PointF originPoint,
            std::string_view text,
            TextScale scale,
            Color color) = 0;

        virtual void drawTexture(
            const ITexture &texture,
            RectF sourceRect,
            RectF destinationRect,
            Color tintColor) = 0;

        [[nodiscard]] ClipScope clipScope(const RectF areaRect)
        {
            beginClip(areaRect);

            return ClipScope{*this};
        }
    };

}
