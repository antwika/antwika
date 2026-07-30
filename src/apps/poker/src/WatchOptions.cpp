#include "antwika/poker/WatchOptions.hpp"

#include <charconv>
#include <chrono>
#include <string_view>
#include <system_error>

namespace antwika::poker
{

    WatchOptions parseWatchOptions(int argc, char **argv)
    {
        WatchOptions options;

        for (int index = 1; index < argc; ++index)
        {
            const std::string_view arg = argv[index];
            if (arg != "--tick-delay-ms" || index + 1 >= argc)
            {
                continue;
            }

            const std::string_view value = argv[++index];
            long long milliseconds = 0;
            const auto parsed = std::from_chars(
                value.data(), value.data() + value.size(), milliseconds);

            if (parsed.ec == std::errc{}
                && parsed.ptr == value.data() + value.size()
                && milliseconds >= 0)
            {
                options.tickDelay = std::chrono::milliseconds{milliseconds};
            }
        }

        return options;
    }

} // namespace antwika::poker
