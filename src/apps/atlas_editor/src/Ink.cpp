#include "antwika/atlas_editor/Ink.hpp"

#include <cstddef>
#include <cstdint>

namespace antwika::atlas_editor
{

    std::uint8_t inkChannelOf(
        const Color ink, const std::size_t channel) noexcept
    {
        if (channel == 0)
        {
            return ink.red;
        }

        return channel == 1 ? ink.green : ink.blue;
    }

    Color withInkChannel(
        Color ink,
        const std::size_t channel,
        const std::uint8_t level) noexcept
    {
        if (channel == 0)
        {
            ink.red = level;
        }
        else if (channel == 1)
        {
            ink.green = level;
        }
        else
        {
            ink.blue = level;
        }

        return ink;
    }

}
