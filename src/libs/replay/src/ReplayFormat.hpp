#pragma once

#include <cstdint>
#include <string_view>

/**
 * @file
 * @brief On-disk format constants shared by ReplayWriter and ReplayReader.
 *
 * Bumping the format version is how the on-disk layout evolves; readers
 * reject anything they don't recognize instead of guessing.
 */
namespace antwika::replay::detail
{

    /**
     * @brief Magic string identifying a replay stream's header line.
     */
    inline constexpr std::string_view kReplayMagic = "antwika-replay";

    /**
     * @brief Version of the on-disk replay format written and expected.
     */
    inline constexpr std::uint32_t kReplayFormatVersion = 1;

} // namespace antwika::replay::detail
