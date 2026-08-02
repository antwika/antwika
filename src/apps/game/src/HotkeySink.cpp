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
        Camera &camera,
        Camera home,
        PauseState &pause) noexcept
        : options(options),
          input(input),
          camera(camera),
          home(home),
          pause(pause)
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
            pause.set(!pause.paused());
            return;
        }

        if (action == Action::ZoomIn)
        {
            camera.zoomIn();
            return;
        }

        if (action == Action::ZoomOut)
        {
            camera.zoomOut();
            return;
        }

        camera = home;
    }

} // namespace antwika::game
