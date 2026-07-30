#pragma once

#include <cstdint>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IRenderer.hpp"
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
    class NullRenderer final : public IRenderer
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
         * @brief Discard a present.
         */
        void present() override;

    private:
        ILogger &logger;
    };

} // namespace antwika::gfx::detail
