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

    /** @brief How many notes the demo track plays. */
    inline constexpr std::size_t kNoteCount = 8;

    /**
     * @brief Build a plucked tone as a waveform.
     *
     * Everything worth testing about this app is in here rather than in
     * main.cpp, which is what lets main stay a composition root: the
     * numbers below are asserted without opening a device at all.
     *
     * @param format The rate and channel count to generate at.
     * @param hertz The pitch.
     * @param frames How long it lasts.
     * @return The tone, complete and ready to be added to a library.
     */
    [[nodiscard]] Waveform demoTone(
        const WaveFormat &format, double hertz, FrameCount frames);

    /**
     * @brief Lay the notes out in time.
     *
     * Every start frame is **absolute**, which is what this whole demo
     * is here to show: a note placed at a frame begins at that frame and
     * not at whichever buffer boundary the device happens to reach next.
     * So the schedule is decided once, before anything is opened, and
     * nothing about how the run is pumped can move a note.
     *
     * @param waveform The tone every note plays.
     * @param spacing How many frames apart the notes begin.
     * @return The notes, in ascending start order.
     */
    [[nodiscard]] std::vector<PlayRequest> demoSchedule(
        WaveformId waveform, FrameCount spacing);

} // namespace antwika::sound_demo
