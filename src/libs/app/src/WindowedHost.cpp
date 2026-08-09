#include "antwika/app/WindowedHost.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace antwika::app
{

    namespace
    {
        template <typename Made, typename Factory>
        [[nodiscard]] std::unique_ptr<Made> madeBy(
            const Factory &factory,
            ILogger &logger,
            const std::string &what)
        {
            if (!factory)
            {
                throw std::invalid_argument(
                    "antwika::app: a windowed host was given no "
                    + what + " factory");
            }

            auto made = factory(logger);

            if (!made)
            {
                throw std::invalid_argument(
                    "antwika::app: a windowed host's " + what
                    + " factory answered with nothing");
            }

            return made;
        }
    }

    WindowedHost::WindowedHost(
        std::ostream &out,
        const Level minimum,
        const BackendFactories &backends,
        const WindowedSessionDesc &desc)
        : logging(out, minimum),
          gfxBackend(madeBy<IGfxBackend>(
              backends.gfx, logging.logger(), "graphics")),
          inputBackend(madeBy<IInputBackend>(
              backends.input, logging.logger(), "input")),
          windowed(logging.logger(), *gfxBackend, *inputBackend, desc)
    {
    }

    ILogger &WindowedHost::logger() noexcept
    {
        return logging.logger();
    }

    WindowedSession &WindowedHost::session() noexcept
    {
        return windowed;
    }

}
