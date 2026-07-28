#pragma once

#include <array>
#include <cstdint>

/**
 * @file
 * @brief On-disk format constants shared by BinaryReplayWriter and
 * BinaryReplayReader.
 *
 * Bumping kReplayFormatVersion is how the on-disk layout evolves; readers
 * reject anything they don't recognize instead of guessing.
 */
namespace antwika::replay::detail
{

    /**
     * @brief Magic bytes identifying a binary replay stream.
     */
    inline constexpr std::array<char, 4> kReplayMagic{'A', 'R', 'P', 'L'};

    /**
     * @brief Version of the on-disk binary replay format written and expected.
     */
    inline constexpr std::uint32_t kReplayFormatVersion = 1;

} // namespace antwika::replay::detail
