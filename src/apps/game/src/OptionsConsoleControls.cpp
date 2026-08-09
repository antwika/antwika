#include "antwika/game/OptionsConsoleControls.hpp"

namespace antwika::game
{

    OptionsConsoleControls::OptionsConsoleControls(
        const OptionsState &options) noexcept
        : options(options)
    {
    }

    antwika::input::Key OptionsConsoleControls::toggleKey() const
    {
        return options.bindings().keyFor(Action::ConsoleToggle);
    }

    antwika::input::Key OptionsConsoleControls::executeKey() const
    {
        return options.bindings().keyFor(Action::ConsoleExecute);
    }

    antwika::console::KeyboardLayout
    OptionsConsoleControls::keyboard() const
    {
        return options.keyboard();
    }

}
