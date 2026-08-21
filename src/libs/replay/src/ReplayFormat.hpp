#pragma once

#include <string_view>

namespace antwika::replay::detail
{

    inline constexpr std::string_view kReplayMagic = "antwika-replay";

    inline constexpr std::string_view kMagicKey = "magic";

    inline constexpr std::string_view kLegacyEventsKey = "events";

}
