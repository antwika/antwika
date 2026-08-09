#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    struct VoiceChain final
    {
        TrackPreset preset{};

        std::string notation{};

        std::size_t notationAt = 0;

        bool pianoroll = false;

        bool waveform = false;

        [[nodiscard]] bool operator==(const VoiceChain &other) const
            = default;
    };

    [[nodiscard]] VoiceChain parseVoiceChain(std::string_view chain);

    [[nodiscard]] std::string_view voiceControls() noexcept;

}
