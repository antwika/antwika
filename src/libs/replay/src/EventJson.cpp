#include "antwika/replay/EventJson.hpp"

namespace antwika::event
{

    void to_json(nlohmann::json &j, const Event &event)
    {
        j["name"] = event.name;
        j["payload"] = event.payload;
    }

    void from_json(const nlohmann::json &j, Event &event)
    {
        event.name = j.at("name").get<std::string>();
        event.payload = j.at("payload").get<std::string>();
    }

    void to_json(nlohmann::json &j, const TickEvent &event)
    {
        j["tick"] = event.tick;
        j["event"] = event.event;
    }

    void from_json(const nlohmann::json &j, TickEvent &event)
    {
        event.tick = j.at("tick").get<antwika::time::Tick>();
        event.event = j.at("event").get<Event>();
    }

}
