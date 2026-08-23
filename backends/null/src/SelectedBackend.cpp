#include "antwika/gfx/SelectedBackend.hpp"

#include <memory>

#include "antwika/gfx/NullBackend.hpp"

namespace antwika::gfx
{

    std::unique_ptr<IGfxBackend> createSelectedBackend(ILogger &logger)
    {
        return std::make_unique<NullBackend>(logger);
    }

}
