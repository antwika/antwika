#pragma once

#include <antwika/input/Key.hpp>

#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console
{

    class IConsoleControls
    {
    public:
        virtual ~IConsoleControls() = default;

        [[nodiscard]] virtual antwika::input::Key toggleKey() const = 0;

        [[nodiscard]] virtual antwika::input::Key
        executeKey() const = 0;

        [[nodiscard]] virtual KeyboardLayout keyboard() const = 0;
    };

    class FixedConsoleControls final : public IConsoleControls
    {
    public:
        explicit FixedConsoleControls(
            antwika::input::Key toggle = antwika::input::Key::Grave,
            antwika::input::Key execute = antwika::input::Key::Enter,
            KeyboardLayout layout = kDefaultKeyboardLayout) noexcept
            : toggle(toggle), execute(execute), layout(layout)
        {
        }

        [[nodiscard]] antwika::input::Key toggleKey() const override
        {
            return toggle;
        }

        [[nodiscard]] antwika::input::Key executeKey() const override
        {
            return execute;
        }

        [[nodiscard]] KeyboardLayout keyboard() const override
        {
            return layout;
        }

    private:
        antwika::input::Key toggle;
        antwika::input::Key execute;
        KeyboardLayout layout;
    };

}
