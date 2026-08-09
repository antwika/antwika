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

    inline constexpr std::size_t kUnlimitedWindows =
        std::numeric_limits<std::size_t>::max();

    class IGfxBackend
    {
    public:
        virtual ~IGfxBackend() = default;

        [[nodiscard]] virtual std::string_view name() const = 0;

        [[nodiscard]] virtual std::size_t maxWindows() const = 0;

        [[nodiscard]] virtual std::unique_ptr<IWindow> createWindow(
            const WindowDesc &desc) = 0;

        [[nodiscard]] virtual std::optional<WindowEvent> pollEvent() = 0;
    };

}
