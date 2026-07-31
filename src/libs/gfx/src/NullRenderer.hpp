#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IRenderer3D.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx::detail
{

    using antwika::log::ILogger;

    /**
     * @brief Renderer that discards every drawing operation.
     *
     * Each operation is logged at trace level rather than being a silent
     * no-op, so a headless run can still show that the render path ran
     * and in what order.
     */
    class NullRenderer final
        : public IRenderer
        , public IRenderer3D
    {
    public:
        /**
         * @brief Construct the renderer.
         * @param logger Receives one trace record per operation.
         */
        explicit NullRenderer(ILogger &logger);

        NullRenderer(const NullRenderer &) = delete;
        NullRenderer(NullRenderer &&) = delete;

        NullRenderer &operator=(const NullRenderer &) = delete;
        NullRenderer &operator=(NullRenderer &&) = delete;

        /**
         * @brief Discard a clear.
         * @param color The colour that would have been filled.
         */
        void clear(Color color) override;

        /**
         * @brief Discard a rectangle fill.
         * @param rect The rectangle that would have been filled.
         * @param color The colour it would have been filled with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Discard a line.
         * @param from The end it would have started at.
         * @param to The end it would have finished at.
         * @param color The colour it would have been drawn in.
         */
        void drawLine(Point from, Point to, Color color) override;

        /**
         * @brief Discard a line of text.
         * @param origin Where the text would have started.
         * @param text The characters that would have been drawn.
         * @param scale The size they would have been drawn at.
         * @param color The colour they would have been drawn in.
         */
        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Create a texture that holds nothing but its size.
         *
         * Still checks the bitmap, so an application that would be
         * refused by a real backend is refused here too.
         * @param bitmap The pixels that would have been uploaded.
         * @return A texture reporting the bitmap's size.
         * @throws GfxError If the bitmap is not complete.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Discard a blit.
         * @param texture The texture that would have been sampled.
         * @param source The region that would have been taken.
         * @param destination The region that would have been filled.
         * @param tint The tint it would have been drawn through.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        /**
         * @brief Offer this renderer's 3D half.
         *
         * The null backend has one, unlike a real backend that has not
         * grown a 3D path yet: a headless run must exercise every call
         * an application makes, and one it could not make here would go
         * untested.
         * @return This renderer, which is also an IRenderer3D.
         */
        [[nodiscard]] IRenderer3D *renderer3d() override;

        /**
         * @brief Create a mesh that holds nothing but its counts.
         *
         * Still checks the data, so an application that would be
         * refused by a real backend is refused here too.
         * @param mesh The geometry that would have been uploaded.
         * @return A mesh reporting that geometry's counts.
         * @throws GfxError If the data is not complete.
         */
        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        /**
         * @brief Discard a mesh draw.
         * @param mesh The geometry that would have been drawn.
         * @param model Where it would have been placed.
         * @param camera What it would have been seen through.
         * @param tint The tint it would have been drawn through.
         */
        void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) override;

        /**
         * @brief Discard a present.
         */
        void present() override;

    private:
        ILogger &logger;
    };

} // namespace antwika::gfx::detail
