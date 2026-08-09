#pragma once

#include <string>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/BitmapRenderer.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    class BitmapWindow final : public IWindow
    {
    public:
        /**
         * @brief Opens a window that draws onto a page.
         *
         * @param logger Told what the window and its renderer decline.
         * @param id The identity the backend gave this window.
         * @param desc The title, size and fullscreen flag to take on.
         * @throws GfxError If either side of the size is zero.
         */
        BitmapWindow(ILogger &logger, WindowId id, const WindowDesc &desc);

        BitmapWindow(const BitmapWindow &) = delete;
        BitmapWindow(BitmapWindow &&) = delete;

        BitmapWindow &operator=(const BitmapWindow &) = delete;
        BitmapWindow &operator=(BitmapWindow &&) = delete;

        [[nodiscard]] WindowId id() const override;

        [[nodiscard]] bool isOpen() const override;

        [[nodiscard]] std::string title() const override;

        [[nodiscard]] Size configuredSize() const override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] bool isFullscreen() const override;

        [[nodiscard]] IRenderer &renderer() override;

        [[nodiscard]] const Bitmap &page() const noexcept;

        void setTitle(std::string_view title) override;

        void setFullscreen(bool fullscreen) override;

        void close() override;

    private:
        ILogger &logger;
        BitmapRenderer bitmapRenderer;
        WindowId windowId;
        std::string windowTitle;
        Size windowSize;
        bool open = true;
        bool filling = false;
    };

}
