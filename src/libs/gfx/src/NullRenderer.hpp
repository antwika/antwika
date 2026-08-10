#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"

namespace antwika::gfx::detail
{

    using antwika::log::ILogger;

    class NullRenderer final : public IRenderer
    {
    public:
        explicit NullRenderer(ILogger &logger);

        NullRenderer(const NullRenderer &) = delete;
        NullRenderer(NullRenderer &&) = delete;

        NullRenderer &operator=(const NullRenderer &) = delete;
        NullRenderer &operator=(NullRenderer &&) = delete;

        void clear(Color color) override;

        void drawRect(RectF rect, Color color) override;

        void drawLine(PointF from, PointF to, Color color) override;

        void drawText(
            PointF origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void drawTexture(
            const ITexture &texture,
            RectF source,
            RectF destination,
            Color tint) override;

        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) override;

        void pushTransform(const Mat4 &transform) override;

        void popTransform() override;

        void present() override;

    private:
        ILogger &logger;
        std::size_t pushed = 0;
    };

}
