#include "antwika/input/SelectedInputBackend.hpp"

#include <memory>

#include "RaylibInputBackend.hpp"

namespace antwika::input
{

    std::unique_ptr<IInputBackend> makeSelectedInputBackend(ILogger &logger)
    {
        return std::make_unique<raylib::RaylibInputBackend>(logger);
    }

} // namespace antwika::input
