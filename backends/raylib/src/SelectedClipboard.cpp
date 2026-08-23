#include "antwika/input/SelectedClipboard.hpp"

#include <memory>

#include "RaylibClipboard.hpp"

namespace antwika::input
{

    std::unique_ptr<IClipboard> createSelectedClipboard(ILogger &logger)
    {
        return std::make_unique<raylib::RaylibClipboard>(logger);
    }

}
