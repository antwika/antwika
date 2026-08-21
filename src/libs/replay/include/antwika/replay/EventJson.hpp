#pragma once

#include <nlohmann/json_fwd.hpp>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::event
{

    void to_json(nlohmann::json &j, const Event &event);

    void from_json(const nlohmann::json &j, Event &event);

    void to_json(nlohmann::json &j, const TickEvent &event);

    void from_json(const nlohmann::json &j, TickEvent &event);

}
