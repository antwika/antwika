#include "antwika/game/SaveCli.hpp"

#include <array>

#include <antwika/cli/CommandLineError.hpp>

namespace antwika::game
{

    namespace
    {
        constexpr std::array kSaveFlags{
            antwika::cli::FlagSpec{
                .name = "--save",
                .valueName = "<path>",
                .help = "Write the session's state to <path> when it "
                        "ends.",
            },
            antwika::cli::FlagSpec{
                .name = "--load",
                .valueName = "<path>",
                .help = "Start the session from the state in <path>.",
            },
        };
    } // namespace

    std::span<const antwika::cli::FlagSpec> saveCliFlags()
    {
        return kSaveFlags;
    }

    SaveCliOptions saveCliOptionsFrom(
        const antwika::cli::CommandLine &parsed)
    {
        // Members assigned one at a time, not built as an aggregate.
        // replayCliOptionsFrom() gives the reason at length.
        // gcov counts one unreachable landing pad per member.
        SaveCliOptions options;
        options.savePath = parsed.value("--save");
        options.loadPath = parsed.value("--load");
        return options;
    } // GCOVR_EXCL_LINE

    void requireRecordableStart(
        const SaveCliOptions &options, const bool recording)
    {
        if (recording && options.loadPath.has_value())
        {
            throw antwika::cli::CommandLineError(
                "--record with --load would record a session whose "
                "replay starts from an empty grid; load nothing, or "
                "replay the session that built the save");
        }
    }

} // namespace antwika::game
