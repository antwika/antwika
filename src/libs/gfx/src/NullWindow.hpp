#pragma once

#include <string>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowId.hpp"

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
         * @param id The id the backend assigned to this window.
         * @param desc The requested title and size.
         */
        NullWindow(ILogger &logger, WindowId id, const WindowDesc &desc);

        NullWindow(const NullWindow &) = delete;
        NullWindow(NullWindow &&) = delete;

        NullWindow &operator=(const NullWindow &) = delete;
        NullWindow &operator=(NullWindow &&) = delete;

        /**
         * @brief Get this window's id.
         * @return The id the backend assigned at creation.
         */
        [[nodiscard]] WindowId id() const override;

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
         * @return WindowDesc::size, unchanged.
         */
        [[nodiscard]] Size configuredSize() const override;

        /**
         * @brief Get the size the window reports.
         * @return The same as configuredSize(), which never changes:
         * nothing can resize a window that is not on a screen, so a
         * headless run honours WindowDesc::resizable by having nothing
         * ever act on it.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Whether this window is filling the screen.
         * @return Whatever was last asked for, starting at
         * WindowDesc::fullscreen. There is no screen to fill, so the
         * request is remembered and nothing acts on it -- exactly as
         * WindowDesc::resizable is honoured here.
         */
        [[nodiscard]] bool isFullscreen() const override;

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
         * @brief Remember whether the window should fill the screen.
         * @param fullscreen What isFullscreen() will answer.
         */
        void setFullscreen(bool fullscreen) override;

        /**
         * @brief Close the window, or do nothing if already closed.
         */
        void close() override;

    private:
        ILogger &logger;
        NullRenderer nullRenderer;
        WindowId windowId;
        std::string windowTitle;
        Size windowSize;
        bool open = true;
        bool filling = false;
    };

} // namespace antwika::gfx::detail
