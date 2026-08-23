#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/GfxCapabilities.hpp"
#include "antwika/gfx/IGfxBackend.hpp"
#include "antwika/gfx/IWindow.hpp"
#include "antwika/gfx/WindowSpec.hpp"
#include "antwika/gfx/WindowEvent.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    class NullBackend final : public IGfxBackend
    {
    public:
        explicit NullBackend(ILogger &logger);

        NullBackend(const NullBackend &) = delete;
        NullBackend(NullBackend &&) = delete;

        NullBackend &operator=(const NullBackend &) = delete;
        NullBackend &operator=(NullBackend &&) = delete;

        [[nodiscard]] std::string_view getName() const override;

        [[nodiscard]] std::size_t getMaxWindows() const override;

        [[nodiscard]] GfxCapabilities getCapabilities()
            const override;

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowSpec &spec) override;

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override;

    private:
        ILogger &logger;
        std::uint64_t nextWindowId = 1;
    };

}
