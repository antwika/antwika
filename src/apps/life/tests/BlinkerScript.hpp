#pragma once

#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/life/Events.hpp"

namespace antwika::life::tests
{

    [[nodiscard]] inline std::vector<antwika::event::TickEvent>
    blinkerScript()
    {
        using antwika::event::Event;
        using antwika::event::TickEvent;

        return {
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kToggleCell,
                    .payload = R"({"x":1,"y":2})",
                },
            },
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kToggleCell,
                    .payload = R"({"x":2,"y":2})",
                },
            },
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kToggleCell,
                    .payload = R"({"x":3,"y":2})",
                },
            },
            TickEvent{
                .tick = 3,
                .event = Event{.name = antwika::engine::events::kStop},
            },
        };
    }

}
