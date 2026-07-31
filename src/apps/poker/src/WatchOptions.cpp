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
            antwika::replay::FlagSpec{
                .name = kTickDelayFlag,
                .valueName = "<n>",
                .help = "Hold each tick's frame for <n> milliseconds "
                        "(default 1000; 0 runs flat out)."}};
    } // namespace

    std::span<const antwika::replay::FlagSpec> watchFlags()
    {
        return kFlags;
    }

    WatchOptions watchOptionsFrom(const antwika::replay::CommandLine &parsed)
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

            // Naming a positive delay is what "I am watching" means.
            // Naming zero is asking for the flat-out run, which ends.
            options.holdFinalFrame = milliseconds > 0;
        }

        return options;
    }

} // namespace antwika::poker
