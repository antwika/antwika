#pragma once

#include <cstdint>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/Events.hpp"

namespace antwika::task_worker::tests
{

    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWorkerCount = 2;

    [[nodiscard]] inline std::vector<antwika::event::TickEvent> demoScript()
    {
        using antwika::event::Event;
        using antwika::event::TickEvent;

        return {
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})"}},
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kTaskSubmit,
                    .payload = R"({"id":2,"priority":1,)"
                               R"("durationTicks":5,"label":"Beta"})"}},
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = events::kTaskSubmit,
                    .payload = R"({"id":3,"priority":0,)"
                               R"("durationTicks":2,"label":"Gamma"})"}},
            TickEvent{
                .tick = 4,
                .event = Event{
                    .name = events::kTaskSubmit,
                    .payload = R"({"id":4,"priority":3,)"
                               R"("durationTicks":1,"label":"Delta"})"}},
            TickEvent{
                .tick = 4,
                .event = Event{
                    .name = events::kTaskSubmit,
                    .payload = R"({"id":5,"priority":1,)"
                               R"("durationTicks":1,"label":"Epsilon",)"
                               R"("dependsOnId":4})"}},
            TickEvent{
                .tick = 5,
                .event = Event{
                    .name = antwika::engine::events::kStop}},
        };
    }

}
