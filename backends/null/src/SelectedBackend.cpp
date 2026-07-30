#include "antwika/gfx/SelectedBackend.hpp"

#include <memory>

#include "antwika/gfx/NullBackend.hpp"

namespace antwika::gfx
{

    std::unique_ptr<IGfxBackend> makeSelectedBackend(ILogger &logger)
    {
        return std::make_unique<NullBackend>(logger);
    }

} // namespace antwika::gfx
