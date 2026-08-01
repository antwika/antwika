#include <iostream>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/app/WavFile.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/SelectedSoundBackend.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/WaveformLibrary.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/sound_demo/DemoLoop.hpp"
#include "antwika/sound_demo/DemoOptions.hpp"
#include "antwika/sound_demo/DemoTrack.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::readWavFile;
using antwika::app::runGuarded;
using antwika::log::Level;
using antwika::sound::DeviceDesc;
using antwika::sound::WaveFormat;
using antwika::sound::WaveformLibrary;
using antwika::sound_demo::demoFlags;
using antwika::sound_demo::demoOptionsFrom;
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

    constexpr std::string_view kName = "antwika_sound_demo";

    return runGuarded(
        kName,
        [&logger, argc, argv]
        {
            // A refused flag is a failed run, not a crash.
            // Parsed outside the guard it reaches std::terminate.
            // That is runRecorded()'s reason, and it holds here too.
            const auto options = demoOptionsFrom(
                antwika::cli::parseCommandLine(argc, argv, demoFlags()));

            // --help is a question, not a run.
            // Answering it opens no device.
            if (options.helpRequested)
            {
                std::cout << antwika::cli::helpText(kName, demoFlags());
                return;
            }

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
            const auto waveform = options.filePath.has_value()
                ? library.add(readWavFile(*options.filePath, kName))
                : library.add(demoTone(kFormat, kPitch, kToneFrames));

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
