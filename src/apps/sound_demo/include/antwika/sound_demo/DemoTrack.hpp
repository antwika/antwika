#pragma once

#include <cstdint>
#include <vector>

#include <antwika/sound/Frames.hpp>
#include <antwika/sound/PlayRequest.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/sound/WaveformId.hpp>

namespace antwika::sound_demo
{

    using antwika::sound::FrameCount;
    using antwika::sound::PlayRequest;
    using antwika::sound::WaveFormat;
    using antwika::sound::Waveform;
    using antwika::sound::WaveformId;

    inline constexpr std::size_t kNoteCount = 8;

    [[nodiscard]] Waveform demoTone(
        const WaveFormat &format, double hertz, FrameCount frames);

    [[nodiscard]] std::vector<PlayRequest> demoSchedule(
        WaveformId waveform, FrameCount spacing);

}
