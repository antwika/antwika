#pragma once

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Keyboard.hpp"
#include "antwika/input/Mouse.hpp"

namespace antwika::input
{

    class InputState final
    {
    public:
        void beginTick() noexcept;

        void apply(const InputEvent &event) noexcept;

        [[nodiscard]] const Keyboard &keyboard() const noexcept;

        [[nodiscard]] const Mouse &mouse() const noexcept;

    private:
        Keyboard keys;
        Mouse pointer;
    };

}
