#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IGfxBackend.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowEvent.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    class BitmapBackend final : public IGfxBackend
    {
    public:
        explicit BitmapBackend(ILogger &logger);

        BitmapBackend(const BitmapBackend &) = delete;
        BitmapBackend(BitmapBackend &&) = delete;

        BitmapBackend &operator=(const BitmapBackend &) = delete;
        BitmapBackend &operator=(BitmapBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] std::size_t maxWindows() const override;

        /**
         * @brief Opens a window that draws onto a page.
         *
         * @param desc The title, size and fullscreen flag to open with.
         * @return The window, whose page() holds what is drawn on it.
         * @throws GfxError If either side of the size is zero.
         */
        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override;

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
        std::uint64_t nextWindowId = 1;
    };

}
