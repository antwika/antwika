#pragma once

#include <string>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowDesc.hpp"

#include "NullRenderer.hpp"

namespace antwika::gfx::detail
{

    using antwika::log::ILogger;

    /**
     * @brief Window that exists, tracks its own properties, and shows
     * nothing.
     */
    class NullWindow final : public IWindow
    {
    public:
        /**
         * @brief Construct the window.
         * @param logger Receives this window's diagnostics.
         * @param desc The requested title and size.
         */
        NullWindow(ILogger &logger, const WindowDesc &desc);

        NullWindow(const NullWindow &) = delete;
        NullWindow(NullWindow &&) = delete;

        NullWindow &operator=(const NullWindow &) = delete;
        NullWindow &operator=(NullWindow &&) = delete;

        /**
         * @brief Whether this window is still open.
         * @return True until close() has been called.
         */
        [[nodiscard]] bool isOpen() const override;

        /**
         * @brief Get the window's current title.
         * @return The title.
         */
        [[nodiscard]] std::string title() const override;

        /**
         * @brief Get the size the window was created with.
         * @return The size in pixels, which never changes: nothing can
         * resize a window that is not on a screen.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Get this window's renderer.
         * @return The same discarding renderer on every call.
         */
        [[nodiscard]] IRenderer &renderer() override;

        /**
         * @brief Replace the window's title.
         * @param title The new title.
         */
        void setTitle(std::string_view title) override;

        /**
         * @brief Close the window, or do nothing if already closed.
         */
        void close() override;

    private:
        ILogger &logger;
        NullRenderer nullRenderer;
        std::string windowTitle;
        Size windowSize;
        bool open = true;
    };

} // namespace antwika::gfx::detail
