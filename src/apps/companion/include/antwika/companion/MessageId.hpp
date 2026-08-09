#pragma once

#include <cstdint>

namespace antwika::companion
{

    enum class MessageId : std::uint16_t
    {
        Hunger,

        Happy,

        Awake,

        AwakeHungry,

        Asleep,

        AsleepWoken,

        Gone,

        NewPet,

        PropFeed,

        PropPlay,

        PropSleep,

        SayHello,

        SayBored,

        SayNiceDay,

        SayLaLaLa,

        SayFeedMe,

        SayYumYum,

        SayFull,

        SayShhh,

        SayZzz,

        SayPlay,

        SayWheee,

        SayTooTired,

        SayNotSleepy,

        SayYawn,

        SayPoked,

        StageEgg,

        StageChild,

        StageTeen,

        StageAdult,

        StageElder,

        MoodHungry,

        MoodRestless,

        MoodHeavy,

        Day,

        Lineage,

        Count,
    };

}
