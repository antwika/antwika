#include "EventSchema.hpp"

#include <antwika/schema/JsonSchemas.hpp>

namespace antwika::replay::detail
{

    using schema::getCountSchema;
    using schema::getWordSchema;

    namespace
    {
        nlohmann::json createTickEventSchema()
        {
            nlohmann::json eventSchema;
            eventSchema["type"] = "object";
            eventSchema["additionalProperties"] = false;
            eventSchema["required"] = {"name", "payload"}; // GCOVR_EXCL_LINE
            eventSchema["properties"]["name"] = getWordSchema();
            eventSchema["properties"]["payload"] = getWordSchema();

            nlohmann::json timedEvent;
            timedEvent["type"] = "object";
            timedEvent["additionalProperties"] = false;
            timedEvent["required"] = {"tick", "event"}; // GCOVR_EXCL_LINE
            timedEvent["properties"]["tick"] = getCountSchema();
            timedEvent["properties"]["event"] = eventSchema;
            return timedEvent;
        }
    }

    const nlohmann::json &getTickEventSchema()
    {
        static const nlohmann::json shape =
            createTickEventSchema(); // GCOVR_EXCL_LINE
        return shape;
    }

}
