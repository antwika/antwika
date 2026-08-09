#include "antwika/sound_demo/DemoOptions.hpp"

#include <array>
#include <string_view>

namespace antwika::sound_demo
{

    namespace
    {
        constexpr std::string_view kFileFlag = "--file";

        constexpr std::array kFlags{
            antwika::cli::FlagSpec{
                .name = kFileFlag,
                .valueName = "<path>",
                .help = "Play this WAV file; without it the demo plays "
                        "a tone it generates."}};
    }

    std::span<const antwika::cli::FlagSpec> demoFlags()
    {
        return kFlags;
    }

    DemoOptions demoOptionsFrom(const antwika::cli::CommandLine &parsed)
    {
        DemoOptions options;
        options.filePath = parsed.value(kFileFlag);
        options.helpRequested = parsed.has(antwika::cli::kHelpFlag);
        return options;
    } // GCOVR_EXCL_LINE

}
