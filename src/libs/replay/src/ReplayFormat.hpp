#pragma once

#include <array>
#include <cstdint>

namespace antwika::replay::detail
{

    // Shared by BinaryReplayWriter and BinaryReplayReader.
    // Bumping kReplayFormatVersion is how the on-disk layout evolves.
    // Readers reject anything they don't recognize instead of guessing.
    inline constexpr std::array<char, 4> kReplayMagic{'A', 'R', 'P', 'L'};
    inline constexpr std::uint32_t kReplayFormatVersion = 1;

} // namespace antwika::replay::detail
