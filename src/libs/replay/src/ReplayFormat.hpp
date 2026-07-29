#pragma once

#include <array>
#include <cstdint>
#include <string_view>

/**
 * @file
 * @brief On-disk format constants shared by the replay writers and
 * readers.
 *
 * Bumping a format version is how its on-disk layout evolves; readers
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

    /**
     * @brief Magic string identifying a JSON replay stream's header line.
     */
    inline constexpr std::string_view kJsonReplayMagic = "antwika-replay";

    /**
     * @brief Version of the on-disk JSON replay format written and expected.
     */
    inline constexpr std::uint32_t kJsonReplayFormatVersion = 1;

} // namespace antwika::replay::detail
