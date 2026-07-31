#include <iostream>
#include <string>
#include <vector>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/app/WavFile.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/SelectedSoundBackend.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/WaveformLibrary.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/sound_demo/DemoLoop.hpp"
#include "antwika/sound_demo/DemoTrack.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::readWavFile;
using antwika::app::runGuarded;
using antwika::log::Level;
using antwika::sound::DeviceDesc;
using antwika::sound::WaveFormat;
using antwika::sound::WaveformLibrary;
using antwika::sound_demo::demoSchedule;
using antwika::sound_demo::DemoLoop;
using antwika::sound_demo::demoTone;
using antwika::time::SystemSleeper;

namespace
{
    constexpr WaveFormat kFormat{.rate = 48000, .channels = 2};

    // One second between notes, and one second each.
    constexpr antwika::sound::FrameCount kSpacing = kFormat.rate / 2;
    constexpr antwika::sound::FrameCount kToneFrames = kFormat.rate / 2;

    constexpr double kPitch = 440.0;

    // Long enough for every note plus the last one's tail.
    constexpr antwika::sound::FrameCount kRunFrames =
        kSpacing * antwika::sound_demo::kNoteCount + kToneFrames;
} // namespace

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    const std::vector<std::string> args(argv + 1, argv + argc);

    return runGuarded(
        "antwika_sound_demo",
        [&logger, &args]
        {
            const auto backend =
                antwika::sound::makeSelectedSoundBackend(logger);

            logger.log(
                Level::Info,
                "Antwika sound demo on backend: "
                    + std::string(backend->name()));

            WaveformLibrary library;

            // A file if one was named, a generated tone otherwise.
            // So the demo needs no asset checked in beside it.
            // And readWavFile keeps a caller that is not a test.
            const auto waveform = args.empty()
                ? library.add(demoTone(kFormat, kPitch, kToneFrames))
                : library.add(readWavFile(args.front(), "antwika_sound_demo"));

            SystemSleeper sleeper;
            DemoLoop loop(*backend, library, sleeper);

            loop.run(
                DeviceDesc{.format = kFormat},
                demoSchedule(waveform, kSpacing),
                kRunFrames);

            logger.log(
                Level::Info,
                "Antwika sound demo rendered "
                    + std::to_string(loop.rendered()) + " frames");
        });
}
