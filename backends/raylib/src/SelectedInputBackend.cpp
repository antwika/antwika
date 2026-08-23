#include "antwika/input/SelectedInputBackend.hpp"

#include <memory>

#include "RaylibInputBackend.hpp"

namespace antwika::input
{

    std::unique_ptr<IInputBackend> createSelectedInputBackend(ILogger &logger)
    {
        return std::make_unique<raylib::RaylibInputBackend>(logger);
    }

}
