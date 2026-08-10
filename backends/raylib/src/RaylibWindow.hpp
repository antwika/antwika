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

    class RaylibWindow final : public IWindow
    {
    public:
        RaylibWindow(
            ILogger &logger,
            RaylibBackend &backend,
            WindowId id,
            const WindowDesc &desc);

        RaylibWindow(const RaylibWindow &) = delete;
        RaylibWindow(RaylibWindow &&) = delete;

        RaylibWindow &operator=(const RaylibWindow &) = delete;
        RaylibWindow &operator=(RaylibWindow &&) = delete;

        ~RaylibWindow() override;

        [[nodiscard]] WindowId id() const override;

        [[nodiscard]] bool isOpen() const override;

        [[nodiscard]] std::string title() const override;

        [[nodiscard]] Size configuredSize() const override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] bool isFullscreen() const override;

        [[nodiscard]] IRenderer &renderer() override;

        void setTitle(std::string_view title) override;

        void setSize(Size size) override;

        void setFullscreen(bool fullscreen) override;

        void close() override;

        [[nodiscard]] std::optional<WindowEvent> takePendingEvent();

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
        bool lastFullscreen = false;
    };

}
