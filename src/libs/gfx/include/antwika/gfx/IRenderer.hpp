#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"

namespace antwika::gfx
{

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void clear(Color color) = 0;

        virtual void drawRect(RectF rect, Color color) = 0;

        virtual void drawLine(PointF from, PointF to, Color color) = 0;

        virtual void drawText(
            PointF origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) = 0;

        [[nodiscard]] virtual std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) = 0;

        virtual void drawTexture(
            const ITexture &texture,
            RectF source,
            RectF destination,
            Color tint) = 0;

        [[nodiscard]] virtual std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) = 0;

        virtual void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) = 0;

        /**
         * @brief Multiplies a transform onto the drawing stack.
         *
         * @param transform Applied to every later call until the
         *                  matching popTransform().
         * @throws GfxError If the stack is deeper than the renderer can
         *         hold.
         *
         * Ensures: every push is undone by exactly one popTransform().
         */
        virtual void pushTransform(const Mat4 &transform) = 0;

        /**
         * @brief Undoes the most recent pushTransform().
         *
         * @throws GfxError If nothing is on the stack to undo.
         */
        virtual void popTransform() = 0;

        virtual void present() = 0;
    };

}
