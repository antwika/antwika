#include "antwika/synth/Oscillate.hpp"

#include <cmath>
#include <cstdint>

#include <antwika/rng/SplitMix64Rng.hpp>

namespace antwika::synth
{

    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        constexpr std::uint64_t kSpread = 0x9E3779B97F4A7C15ULL;

        constexpr std::uint64_t kNoiseShift = 40;
        constexpr double kNoiseScale = 16777216.0;

        [[nodiscard]] float noiseAt(
            std::uint64_t seed, FrameCount position) noexcept
        {
            rng::SplitMix64Rng generator(seed ^ (position * kSpread));

            const auto bits = generator.next();

            const auto unit = static_cast<double>(bits >> kNoiseShift)
                / kNoiseScale;

            return static_cast<float>(2.0 * unit - 1.0);
        }
    }

    float oscillate(
        Waveshape shape,
        double phase,
        std::uint64_t seed,
        FrameCount position) noexcept
    {
        switch (shape)
        {
        case Waveshape::Sine:
            return static_cast<float>(std::sin(2.0 * kPi * phase));

        case Waveshape::Saw:
            return static_cast<float>(2.0 * phase - 1.0);

        case Waveshape::Square:
            return phase < 0.5 ? 1.0F : -1.0F;

        case Waveshape::Triangle:
            return static_cast<float>(
                phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase);

        case Waveshape::Noise:
            return noiseAt(seed, position);
        }

        return 0.0F;
    }

}
