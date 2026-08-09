#pragma once

#include <cstdint>

namespace antwika::companion
{

    enum class Saying : std::uint8_t
    {
        None = 0,

        Hello,

        Bored,

        NiceDay,

        Silly,

        FeedMe,

        Yum,

        NotHungry,

        LetMeSleep,

        Zzz,

        PlayWithMe,

        Wheee,

        TooTired,

        NotSleepy,

        Yawn,

        Poked,
    };

    [[nodiscard]] constexpr Saying enumBound(Saying) noexcept
    {
        return Saying::Poked;
    }

}
