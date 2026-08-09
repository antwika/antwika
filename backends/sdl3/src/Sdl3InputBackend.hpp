#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Pump.hpp"

namespace antwika::input::sdl3
{

    using antwika::log::ILogger;

    class Sdl3InputBackend final : public IInputBackend
    {
    public:
        explicit Sdl3InputBackend(ILogger &logger);

        Sdl3InputBackend(const Sdl3InputBackend &) = delete;
        Sdl3InputBackend(Sdl3InputBackend &&) = delete;

        Sdl3InputBackend &operator=(const Sdl3InputBackend &) = delete;
        Sdl3InputBackend &operator=(Sdl3InputBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] InputCapabilities capabilities() const override;

        [[nodiscard]] std::optional<InputEvent> pollEvent() override;

    private:
        std::shared_ptr<antwika::sdl3::Sdl3Pump> pump;

        float remainderX = 0.0F;
        float remainderY = 0.0F;
    };

}
