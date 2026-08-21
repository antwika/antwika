#include "antwika/input/SelectedClipboard.hpp"

#include <memory>

#include "antwika/input/InMemoryClipboard.hpp"

namespace antwika::input
{

    std::unique_ptr<IClipboard> makeSelectedClipboard(ILogger &)
    {
        return std::make_unique<InMemoryClipboard>();
    }

}
