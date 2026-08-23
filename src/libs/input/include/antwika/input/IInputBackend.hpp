#pragma once

#include <optional>
#include <string_view>

#include "antwika/input/InputCapabilities.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    class IInputBackend
    {
    public:
        virtual ~IInputBackend() = default;

        [[nodiscard]] virtual std::string_view getName() const = 0;

        [[nodiscard]] virtual InputCapabilities getCapabilities() const = 0;

        [[nodiscard]] virtual std::optional<InputEvent> pollEvent() = 0;
    };

}
