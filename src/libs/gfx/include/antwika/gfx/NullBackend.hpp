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
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    /**
     * @brief Backend whose windows exist and draw nothing.
     *
     * Not a placeholder for a real backend: it is what lets tests, CI and
     * replay verification run with no display and no graphics framework
     * installed. A replay recorded against a real backend must reproduce
     * the same state under this one, which only holds because rendering
     * never feeds back into the simulation.
     *
     * It reports no events, so a program driven purely by this backend
     * ends when its own stop condition says so, not when a user closes a
     * window.
     */
    class NullBackend final : public IGfxBackend
    {
    public:
        /**
         * @brief Construct the backend.
         * @param logger Receives the backend's diagnostics.
         */
        explicit NullBackend(ILogger &logger);

        NullBackend(const NullBackend &) = delete;
        NullBackend(NullBackend &&) = delete;

        NullBackend &operator=(const NullBackend &) = delete;
        NullBackend &operator=(NullBackend &&) = delete;

        /**
         * @brief Get the backend's name.
         * @return Always "null".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief How many windows this backend allows at once.
         * @return kUnlimitedWindows: nothing here is a real resource.
         */
        [[nodiscard]] std::size_t maxWindows() const override;

        /**
         * @brief Open a new window that draws nothing.
         * @param desc What the window should look like.
         * @return The new window, never null.
         * @throws GfxError If desc asks for a zero width or height.
         */
        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) override;

        /**
         * @brief Take the next reported event.
         * @return Always nullopt: there is no window system to report one.
         */
        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
        std::uint64_t nextWindowId = 1;
    };

} // namespace antwika::gfx
