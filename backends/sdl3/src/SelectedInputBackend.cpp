#include "antwika/input/SelectedInputBackend.hpp"

#include <memory>

#include "Sdl3InputBackend.hpp"

namespace antwika::input
{

    std::unique_ptr<IInputBackend> makeSelectedInputBackend(ILogger &logger)
    {
        return std::make_unique<sdl3::Sdl3InputBackend>(logger);
    }

} // namespace antwika::input
