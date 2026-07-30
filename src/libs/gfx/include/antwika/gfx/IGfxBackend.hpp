#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>

#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowEvent.hpp"

namespace antwika::gfx
{

    /**
     * @brief The window count reported by a backend with no fixed limit.
     */
    inline constexpr std::size_t kUnlimitedWindows =
        std::numeric_limits<std::size_t>::max();

    /**
     * @brief Creates windows and reports what happened to them.
     *
     * The one seam between Antwika and a concrete graphics framework.
     * Exactly one implementation is compiled into a given build, chosen
     * by ANTWIKA_GFX_BACKEND, so no code above this interface names SDL,
     * raylib or anything like them.
     */
    class IGfxBackend
    {
    public:
        virtual ~IGfxBackend() = default;

        /**
         * @brief Get the backend's name, for logs and diagnostics.
         * @return A stable identifier, e.g. "null".
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief How many windows this backend can have open at once.
         *
         * Not every graphics framework does multiple windows: raylib, for
         * one, has a single global window and no concept of a second.
         * Rather than have such a backend pretend otherwise, it says so
         * here, and createWindow() refuses to exceed what it reports.
         *
         * @return The limit, or kUnlimitedWindows when there isn't one.
         * Never zero.
         */
        [[nodiscard]] virtual std::size_t maxWindows() const = 0;

        /**
         * @brief Open a new window.
         *
         * Windows are independent as far as the backend allows: where
         * more than one can be open, opening a second does not disturb
         * the first, and either can be closed on its own.
         *
         * The returned window owns itself and may outlive this backend.
         * Destroying it afterwards is safe; drawing into it is not
         * guaranteed to reach a screen.
         *
         * @param desc What the window should look like.
         * @return The new window, never null.
         * @throws GfxError If the window could not be created, including
         * when desc asks for a zero width or height, or when maxWindows()
         * are already open.
         */
        [[nodiscard]] virtual std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) = 0;

        /**
         * @brief Take the next event reported since the last call.
         *
         * Never blocks: an empty queue is reported, not waited on.
         *
         * @return The next event, or nullopt when none is pending.
         */
        [[nodiscard]] virtual std::optional<WindowEvent> pollEvent() = 0;
    };

} // namespace antwika::gfx
