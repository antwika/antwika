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
        // Whatever the fold was just given, since it runs first.
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
        // An if-chain rather than a switch over the enumeration.
        // A switch carries an arm for a value no enumerator names.
        // Which only a cast could reach, and act() is private.
        // The last action is the fall-through, so nothing is left over.
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

        // The console's own two actions fall through to nothing.
        // ConsoleSink acts on them, open or closed.
        // A tail that did anything would make Enter a reset key.
    }

} // namespace antwika::game
