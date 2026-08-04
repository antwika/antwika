#include "antwika/console/ConsoleMount.hpp"

namespace antwika::console
{

    ConsoleMount::ConsoleMount(const ConsoleMountSetup &setup)
        : isMounted(setup.overlay.has_value()),
          picture(isMounted ? setup.overlay->get() : noConsole),
          input(setup.input),
          controls(
              setup.controls.has_value() ? setup.controls->get()
                                         : fixedControls),
          commands(setup.store, setup.dumpPath, setup.loadEnabled),
          // The excluded line is the setup temporary's unwind block.
          // Its members are references alone.
          // So there is nothing to unwind and no input reaches it.
          // See docs/confirming-unreachable-branches.md.
          consoleSink(ConsoleSinkSetup{ // GCOVR_EXCL_LINE
              .console = console,
              .input = input,
              .picture = picture,
              .scene = scene,
              .controls = controls,
              .commands = commands})
    {
    }

    bool ConsoleMount::mounted() const noexcept
    {
        return isMounted;
    }

    ConsoleSink &ConsoleMount::sink() noexcept
    {
        return consoleSink;
    }

    ConsoleState &ConsoleMount::state() noexcept
    {
        return console;
    }

    ConsoleGatedSink ConsoleMount::gate(
        ITickEventSink &inner) const noexcept
    {
        return ConsoleGatedSink(inner, console, input);
    }

} // namespace antwika::console
