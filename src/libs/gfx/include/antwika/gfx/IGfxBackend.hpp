#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/WindowDesc.hpp"
#include "antwika/gfx/WindowEvent.hpp"

namespace antwika::gfx
{

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
         * @brief Open a new window.
         *
         * Windows are independent: opening a second one does not disturb
         * the first, and either can be closed on its own.
         *
         * @param desc What the window should look like.
         * @return The new window, never null.
         * @throws GfxError If the window could not be created, including
         * when desc asks for a zero width or height.
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
