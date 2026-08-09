#include "antwika/game/HotkeySink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>

#include "antwika/game/Action.hpp"

namespace antwika::game
{

    using antwika::input::KeyPressed;

    HotkeySink::HotkeySink(
        const OptionsState &options,
        const InputFold &input,
        ViewCommands &view) noexcept
        : options(options), input(input), view(view)
    {
    }

    void HotkeySink::handle(const TickEvent &)
    {
        const auto &decoded = input.current();

        if (!decoded.has_value())
        {
            return;
        }

        const auto *pressed = std::get_if<KeyPressed>(&*decoded);

        if (pressed == nullptr || pressed->repeat)
        {
            return;
        }

        const auto action = options.bindings().actionFor(pressed->key);

        if (!action.has_value())
        {
            return;
        }

        act(*action);
    }

    void HotkeySink::act(Action action) noexcept
    {
        if (action == Action::Pause)
        {
            view.togglePause();
            return;
        }

        if (action == Action::ZoomIn)
        {
            view.zoomIn();
            return;
        }

        if (action == Action::ZoomOut)
        {
            view.zoomOut();
            return;
        }

        if (action == Action::ResetView)
        {
            view.resetView();
            return;
        }

    }

}
