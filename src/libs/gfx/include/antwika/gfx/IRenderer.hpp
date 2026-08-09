#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IRenderer3D.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void clear(Color color) = 0;

        virtual void drawRect(Rect rect, Color color) = 0;

        virtual void drawLine(Point from, Point to, Color color) = 0;

        virtual void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) = 0;

        [[nodiscard]] virtual std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) = 0;

        virtual void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) = 0;

        [[nodiscard]] virtual IRenderer3D *renderer3d()
        {
            return nullptr;
        }

        virtual void present() = 0;
    };

}
