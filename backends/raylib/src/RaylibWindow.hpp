#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/ILogger.hpp>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    using antwika::log::ILogger;

    class RaylibBackend;

    /**
     * @brief raylib's one and only window.
     *
     * raylib keeps the window in global state, so this object owns
     * something it does not hold a handle to: constructing it calls
     * InitWindow, and close() calls CloseWindow. The backend is told when
     * that happens so it can let a later createWindow() succeed.
     */
    class RaylibWindow final : public IWindow
    {
    public:
        /**
         * @brief Open raylib's window.
         * @param logger Receives this window's diagnostics.
         * @param backend Told when this window closes.
         * @param id The id the backend assigned.
         * @param desc The requested title and size.
         * @throws GfxError If raylib did not come up.
         */
        RaylibWindow(
            ILogger &logger,
            RaylibBackend &backend,
            WindowId id,
            const WindowDesc &desc);

        RaylibWindow(const RaylibWindow &) = delete;
        RaylibWindow(RaylibWindow &&) = delete;

        RaylibWindow &operator=(const RaylibWindow &) = delete;
        RaylibWindow &operator=(RaylibWindow &&) = delete;

        /**
         * @brief Close raylib's window, if still open.
         */
        ~RaylibWindow() override;

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
         * @return The title, which raylib cannot be asked for, so the
         * last one set is remembered here.
         */
        [[nodiscard]] std::string title() const override;

        /**
         * @brief Get the size the window was created with.
         * @return WindowDesc::size, unchanged. raylib's window flags are
         * global and outlive a window, so this is the only size that is
         * safe to say belongs to this window in particular.
         */
        [[nodiscard]] Size configuredSize() const override;

        /**
         * @brief Get the size of the window's drawable area.
         * @return The size raylib reports, or the last one seen if closed.
         * On a resizable window this follows the user around.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Get the renderer that draws into this window.
         * @return The renderer, which discards drawing once closed.
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

        /**
         * @brief Translate raylib's window state into one event.
         *
         * raylib has no event queue, only state to look at, and that
         * state stays set for as long as it is true rather than being
         * consumed by reading it. Every event this reports is therefore
         * latched against what was last reported, so a caller draining
         * the queue reaches the end of it.
         *
         * @return The next event, or nullopt when there is nothing new.
         */
        [[nodiscard]] std::optional<WindowEvent> takePendingEvent();

        /**
         * @brief Stop pointing at a backend that is being destroyed.
         *
         * A window may outlive the backend that made it, and closing one
         * tells its backend so. Without this that call would land on a
         * destroyed object.
         */
        void forgetBackend();

    private:
        ILogger &logger;
        RaylibBackend *backend;
        RaylibRenderer raylibRenderer;
        WindowId windowId;
        std::string windowTitle;
        Size requestedSize;
        Size lastSize;
        bool open = true;
        bool closeReported = false;
    };

} // namespace antwika::gfx::raylib
