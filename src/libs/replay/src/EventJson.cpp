#include "antwika/replay/EventJson.hpp"

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/ReplayFormatError.hpp>

#include "EventSchema.hpp"

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

    void to_json(nlohmann::json &j, const TimedEvent &event)
    {
        j["tick"] = event.tick;
        j["event"] = event.event;
    }

    void from_json(const nlohmann::json &j, TimedEvent &event)
    {
        event.tick = j.at("tick").get<antwika::time::Tick>();
        event.event = j.at("event").get<Event>();
    }

} // namespace antwika::event

namespace antwika::replay
{

    namespace
    {
        nlohmann::json eventSchema()
        {
            nlohmann::json schema = detail::timedEventShape();
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay timed event";
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &eventValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                eventSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    antwika::event::TimedEvent eventFromJson(const nlohmann::json &j)
    {
        try
        {
            eventValidator().validate(j);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ReplayFormatError(
                std::string(
                    "antwika::replay: event JSON failed schema "
                    "validation: ") +
                error.what());
        }
        return j.get<antwika::event::TimedEvent>();
    }

    nlohmann::json eventToJson(const antwika::event::TimedEvent &event)
    {
        return nlohmann::json(event);
    }

} // namespace antwika::replay
