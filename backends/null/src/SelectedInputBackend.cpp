#include "antwika/input/SelectedInputBackend.hpp"

#include <memory>

#include "antwika/input/NullInputBackend.hpp"

namespace antwika::input
{

    std::unique_ptr<IInputBackend> makeSelectedInputBackend(ILogger &logger)
    {
        return std::make_unique<NullInputBackend>(logger);
    }

} // namespace antwika::input
