#pragma once

#include <string>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowSpec.hpp"
#include "antwika/gfx/WindowId.hpp"

#include "NullRenderer.hpp"

namespace antwika::gfx::detail
{

    using antwika::log::ILogger;

    class NullWindow final : public IWindow
    {
    public:
        NullWindow(ILogger &logger, WindowId idWindow, const WindowSpec &spec);

        NullWindow(const NullWindow &) = delete;
        NullWindow(NullWindow &&) = delete;

        NullWindow &operator=(const NullWindow &) = delete;
        NullWindow &operator=(NullWindow &&) = delete;

        [[nodiscard]] WindowId getId() const override;

        [[nodiscard]] bool isOpen() const override;

        [[nodiscard]] std::string getTitle() const override;

        [[nodiscard]] Size getConfiguredSize() const override;

        [[nodiscard]] Size getSize() const override;

        [[nodiscard]] bool isFullscreen() const override;

        [[nodiscard]] IRenderer &renderer() override;

        void setTitle(std::string_view title) override;

        void setSize(Size size) override;

        void setFullscreen(bool fullscreen) override;

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

}
