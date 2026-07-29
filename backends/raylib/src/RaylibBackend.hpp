#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::raylib
{

    using antwika::log::ILogger;

    class RaylibWindow;

    /**
     * @brief IGfxBackend backed by raylib.
     *
     * raylib holds its window, renderer and input state in globals, so
     * only one window can exist at a time; maxWindows() says so instead of
     * pretending otherwise. There is no event queue either -- raylib
     * offers state to inspect -- so pollEvent() asks the live window to
     * translate that state into at most one event per call.
     */
    class RaylibBackend final : public IGfxBackend
    {
    public:
        /**
         * @brief Construct the backend.
         *
         * raylib has no separate initialisation step: the window is the
         * subsystem, so nothing global happens until createWindow().
         *
         * @param logger Receives the backend's diagnostics.
         */
        explicit RaylibBackend(ILogger &logger);

        RaylibBackend(const RaylibBackend &) = delete;
        RaylibBackend(RaylibBackend &&) = delete;

        RaylibBackend &operator=(const RaylibBackend &) = delete;
        RaylibBackend &operator=(RaylibBackend &&) = delete;

        ~RaylibBackend() override = default;

        /**
         * @brief Get the backend's name.
         * @return Always "raylib".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief How many windows this backend allows at once.
         * @return Always 1: raylib has exactly one global window.
         */
        [[nodiscard]] std::size_t maxWindows() const override;

        /**
         * @brief Open raylib's window.
         * @param desc What the window should look like.
         * @return The new window, never null.
         * @throws GfxError If desc asks for a zero width or height, if a
         * window is already open, or if raylib did not come up.
         */
        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override;

        /**
         * @brief Take the next event the live window has to report.
         * @return The next event, or nullopt when there is none.
         */
        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

        /**
         * @brief Note that a window has closed and raylib is free again.
         * @param window The window that closed.
         */
        void forgetWindow(const RaylibWindow &window);

    private:
        ILogger &logger;
        RaylibWindow *live = nullptr;
        std::uint64_t nextWindowId = 1;
    };

} // namespace antwika::gfx::raylib
