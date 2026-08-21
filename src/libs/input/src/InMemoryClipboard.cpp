#include "antwika/input/InMemoryClipboard.hpp"

namespace antwika::input
{

    std::string InMemoryClipboard::text() const
    {
        return heldText;
    }

    void InMemoryClipboard::setText(const std::string_view text)
    {
        heldText = text;
    }

}
