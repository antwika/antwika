#include "antwika/gfx/SelectedBackend.hpp"

#include <memory>

#include "RaylibBackend.hpp"

namespace antwika::gfx
{

    std::unique_ptr<IGfxBackend> makeSelectedBackend(ILogger &logger)
    {
        return std::make_unique<raylib::RaylibBackend>(logger);
    }

} // namespace antwika::gfx
