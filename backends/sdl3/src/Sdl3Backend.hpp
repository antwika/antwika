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

    /**
     * @brief IGfxBackend backed by SDL3.
     *
     * Holds a share of Sdl3Pump for its whole lifetime, which is what
     * keeps SDL's video subsystem up and, more importantly, what keeps
     * this backend and an Sdl3InputBackend from stealing each other's
     * events off the one queue SDL has -- see Sdl3Pump.
     */
    class Sdl3Backend final : public IGfxBackend
    {
    public:
        /**
         * @brief Take a share of the process's SDL event pump.
         * @param logger Receives the backend's diagnostics.
         * @throws GfxError If SDL's video subsystem failed to start.
         */
        explicit Sdl3Backend(ILogger &logger);

        Sdl3Backend(const Sdl3Backend &) = delete;
        Sdl3Backend(Sdl3Backend &&) = delete;

        Sdl3Backend &operator=(const Sdl3Backend &) = delete;
        Sdl3Backend &operator=(Sdl3Backend &&) = delete;

        /**
         * @brief Let go of the pump, shutting SDL down with the last one.
         */
        ~Sdl3Backend() override;

        /**
         * @brief Get the backend's name.
         * @return Always "sdl3".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief How many windows this backend allows at once.
         * @return kUnlimitedWindows: SDL is happy with many.
         */
        [[nodiscard]] std::size_t maxWindows() const override;

        /**
         * @brief Open a new SDL window with a renderer attached.
         * @param desc What the window should look like.
         * @return The new window, never null.
         * @throws GfxError If desc asks for a zero width or height, or if
         * SDL could not create the window or its renderer.
         */
        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override;

        /**
         * @brief Take the next window event SDL has for us.
         *
         * Window events SDL reports that this abstraction has no
         * equivalent for are dropped, and polling continues, so the queue
         * always drains.
         *
         * @return The next translatable event, or nullopt when none is
         * left.
         */
        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
        std::shared_ptr<antwika::sdl3::Sdl3Pump> pump;
    };

} // namespace antwika::gfx::sdl3
