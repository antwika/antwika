#include "antwika/replay/EventJson.hpp"

#include <format>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>

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

} // namespace antwika::event

namespace antwika::replay
{

    namespace
    {
        nlohmann::json eventSchema()
        {
            nlohmann::json schema = detail::tickEventShape();
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay tick event";

            // The version rides in the schema's identity.
            // One event is repeated thousands of times per document.
            // Its revision is fixed by the document holding it.
            // A per-event version would be that many copies of one fact.
            schema["$id"] = std::format(
                "https://antwika.dev/schemas/tick-event/{}",
                kTickEventSchemaVersion);
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &eventValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                eventSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    antwika::event::TickEvent eventFromJson(const nlohmann::json &j)
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
        return j.get<antwika::event::TickEvent>();
    }

    nlohmann::json eventToJson(const antwika::event::TickEvent &event)
    {
        return nlohmann::json(event);
    }

} // namespace antwika::replay
