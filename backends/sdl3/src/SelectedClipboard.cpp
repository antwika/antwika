#include "antwika/input/SelectedClipboard.hpp"

#include <memory>

#include "Sdl3Clipboard.hpp"

namespace antwika::input
{

    std::unique_ptr<IClipboard> makeSelectedClipboard(ILogger &logger)
    {
        return std::make_unique<sdl3::Sdl3Clipboard>(logger);
    }

} // namespace antwika::input
