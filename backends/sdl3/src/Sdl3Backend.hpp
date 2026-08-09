#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Pump.hpp"

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    class Sdl3Backend final : public IGfxBackend
    {
    public:
        explicit Sdl3Backend(ILogger &logger);

        Sdl3Backend(const Sdl3Backend &) = delete;
        Sdl3Backend(Sdl3Backend &&) = delete;

        Sdl3Backend &operator=(const Sdl3Backend &) = delete;
        Sdl3Backend &operator=(Sdl3Backend &&) = delete;

        ~Sdl3Backend() override;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] std::size_t maxWindows() const override;

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override;

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
        std::shared_ptr<antwika::sdl3::Sdl3Pump> pump;
    };

}
