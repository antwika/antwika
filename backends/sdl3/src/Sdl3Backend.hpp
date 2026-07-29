#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief IGfxBackend backed by SDL3.
     *
     * Owns SDL's video subsystem for its whole lifetime: one backend, one
     * SDL_Init/SDL_Quit pair.
     */
    class Sdl3Backend final : public IGfxBackend
    {
    public:
        /**
         * @brief Initialise SDL's video subsystem.
         * @param logger Receives the backend's diagnostics.
         * @throws GfxError If SDL's video subsystem failed to start.
         */
        explicit Sdl3Backend(ILogger &logger);

        Sdl3Backend(const Sdl3Backend &) = delete;
        Sdl3Backend(Sdl3Backend &&) = delete;

        Sdl3Backend &operator=(const Sdl3Backend &) = delete;
        Sdl3Backend &operator=(Sdl3Backend &&) = delete;

        /**
         * @brief Shut SDL's video subsystem down.
         */
        ~Sdl3Backend() override;

        /**
         * @brief Get the backend's name.
         * @return Always "sdl3".
         */
        [[nodiscard]] std::string_view name() const override;

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
         * @brief Take the next event SDL has for us.
         *
         * Events SDL reports that this abstraction has no equivalent for
         * are dropped, and polling continues, so the queue always drains.
         *
         * @return The next translatable event, or nullopt when none is
         * left.
         */
        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
    };

} // namespace antwika::gfx::sdl3
