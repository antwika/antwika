#pragma once

#include <antwika/input/Key.hpp>

#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console::fakes
{

    struct FakeConsoleControls final : IConsoleControls
    {
        antwika::input::Key toggle = antwika::input::Key::F1;

        [[nodiscard]] antwika::input::Key toggleKey() const override
        {
            return toggle;
        }

        [[nodiscard]] antwika::input::Key executeKey() const override
        {
            return antwika::input::Key::Enter;
        }

        [[nodiscard]] KeyboardLayout keyboard() const override
        {
            return kDefaultKeyboardLayout;
        }
    };

}
