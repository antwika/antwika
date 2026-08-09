#include "antwika/poker/WatchOptions.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <string_view>
#include <system_error>

namespace antwika::poker
{

    namespace
    {
        constexpr std::string_view kTickDelayFlag = "--tick-delay-ms";

        constexpr std::array kFlags{
            antwika::cli::FlagSpec{
                .name = kTickDelayFlag,
                .valueName = "<n>",
                .help = "Hold each tick's frame for <n> milliseconds "
                        "(default 1000; 0 runs flat out)."}};
    }

    std::span<const antwika::cli::FlagSpec> watchFlags()
    {
        return kFlags;
    }

    WatchOptions watchOptionsFrom(const antwika::cli::CommandLine &parsed)
    {
        WatchOptions options;

        const auto given = parsed.value(kTickDelayFlag);
        if (!given)
        {
            return options;
        }

        const std::string_view value = *given;
        long long milliseconds = 0;
        const auto read = std::from_chars(
            value.data(), value.data() + value.size(), milliseconds);

        if (read.ec == std::errc{}
            && read.ptr == value.data() + value.size()
            && milliseconds >= 0)
        {
            options.tickDelay = std::chrono::milliseconds{milliseconds};

            options.holdFinalFrame = milliseconds > 0;
        }

        return options;
    }

}
