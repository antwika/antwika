#include "antwika/synth/Waveshape.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::synth
{

    namespace
    {
        constexpr std::array<std::string_view, kWaveshapeCount> kNames{
            "sine",
            "saw",
            "square",
            "triangle",
            "noise",
        };
    }

    std::string_view waveshapeName(Waveshape shape) noexcept
    {
        const std::size_t index = waveshapeIndex(shape);

        if (index >= kNames.size())
        {
            return "unknown";
        }

        return kNames[index];
    }

}
