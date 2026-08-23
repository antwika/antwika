#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::raylib
{

    using antwika::log::ILogger;

    class RaylibWindow;

    class RaylibBackend final : public IGfxBackend
    {
    public:
        explicit RaylibBackend(ILogger &logger);

        RaylibBackend(const RaylibBackend &) = delete;
        RaylibBackend(RaylibBackend &&) = delete;

        RaylibBackend &operator=(const RaylibBackend &) = delete;
        RaylibBackend &operator=(RaylibBackend &&) = delete;

        ~RaylibBackend() override;

        [[nodiscard]] std::string_view getName() const override;

        [[nodiscard]] std::size_t getMaxWindows() const override;

        [[nodiscard]] GfxCapabilities getCapabilities()
            const override;

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowSpec &spec) override;

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

        void untrackWindow(const RaylibWindow &window);

    private:
        ILogger &logger;
        RaylibWindow *openWindow = nullptr;
        std::uint64_t nextWindowId = 1;
    };

}
