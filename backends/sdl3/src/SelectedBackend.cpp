#include "antwika/gfx/SelectedBackend.hpp"

#include <memory>

#include "Sdl3Backend.hpp"

namespace antwika::gfx
{

    std::unique_ptr<IGfxBackend> makeSelectedBackend(ILogger &logger)
    {
        return std::make_unique<sdl3::Sdl3Backend>(logger);
    }

}
