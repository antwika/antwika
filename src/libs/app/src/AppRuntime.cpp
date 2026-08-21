#include "antwika/app/AppRuntime.hpp"

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

            auto runtime = factory(logger);

            if (!runtime)
            {
                throw std::invalid_argument(
                    "antwika::app: a windowed host's " + what
                    + " factory answered with nothing");
            }

            return runtime;
        }
    }

    AppRuntime::AppRuntime(
        std::ostream &outputStream,
        const Level minimumLevel,
        const BackendFactories &backends,
        const WindowedSessionSpec &spec)
        : logging(outputStream, minimumLevel),
          gfxBackend(madeBy<IGfxBackend>(
              backends.gfx, logging.logger(), "graphics")),
          inputBackend(madeBy<IInputBackend>(
              backends.input, logging.logger(), "input")),
          windowedSession(logging.logger(), *gfxBackend, *inputBackend, spec)
    {
    }

    ILogger &AppRuntime::logger() noexcept
    {
        return logging.logger();
    }

    WindowedSession &AppRuntime::session() noexcept
    {
        return windowedSession;
    }

}
