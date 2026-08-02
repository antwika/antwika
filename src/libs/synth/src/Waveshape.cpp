#include "antwika/synth/Waveshape.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::synth
{

    namespace
    {
        // Indexed by waveshapeIndex(), so the order is the enumeration's.
        constexpr std::array<std::string_view, kWaveshapeCount> kNames{
            "sine",
            "saw",
            "square",
            "triangle",
            "noise",
        };
    } // namespace

    std::string_view waveshapeName(Waveshape shape) noexcept
    {
        const std::size_t index = waveshapeIndex(shape);

        // Total rather than throwing, following antwika::tween.
        // This is for a message somebody is already reading.
        // A name that threw would take the program with it.
        if (index >= kNames.size())
        {
            return "unknown";
        }

        return kNames[index];
    }

} // namespace antwika::synth
