#pragma once

#include <antwika/event/EventName.hpp>
#include <antwika/event/EventNameSeeds.hpp>

namespace antwika::input::events
{

    inline constexpr antwika::event::EventName kKeyDown =
        antwika::event::EventName::getSeeded(antwika::event::kKeyDownText);

    inline constexpr antwika::event::EventName kKeyUp =
        antwika::event::EventName::getSeeded(antwika::event::kKeyUpText);

    inline constexpr antwika::event::EventName kPointerMove =
        antwika::event::EventName::getSeeded(antwika::event::kPointerMoveText);

    inline constexpr antwika::event::EventName kPointerDown =
        antwika::event::EventName::getSeeded(antwika::event::kPointerDownText);

    inline constexpr antwika::event::EventName kPointerUp =
        antwika::event::EventName::getSeeded(antwika::event::kPointerUpText);

    inline constexpr antwika::event::EventName kPointerScroll =
        antwika::event::EventName::getSeeded(
            antwika::event::kPointerScrollText);

}
