#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::input::raylib
{

    using antwika::log::ILogger;

    class RaylibInputBackend final : public IInputBackend
    {
    public:
        explicit RaylibInputBackend(ILogger &logger);

        RaylibInputBackend(const RaylibInputBackend &) = delete;
        RaylibInputBackend(RaylibInputBackend &&) = delete;

        RaylibInputBackend &operator=(const RaylibInputBackend &) = delete;
        RaylibInputBackend &operator=(RaylibInputBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] InputCapabilities capabilities() const override;

        [[nodiscard]] std::optional<InputEvent> pollEvent() override;

    private:
        void sample();

        std::deque<InputEvent> pending;
        std::optional<Position> lastPosition;
        std::array<bool, kMouseButtonCount> held{};

        std::uint64_t wheelFrame = ~std::uint64_t{0};

        float remainderX = 0.0F;
        float remainderY = 0.0F;
    };

}
