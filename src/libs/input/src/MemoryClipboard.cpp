#include "antwika/input/MemoryClipboard.hpp"

namespace antwika::input
{

    std::string MemoryClipboard::text() const
    {
        return held;
    }

    void MemoryClipboard::setText(const std::string_view text)
    {
        held = text;
    }

}
