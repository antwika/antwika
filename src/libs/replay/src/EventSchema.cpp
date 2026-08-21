#include "EventSchema.hpp"

#include <antwika/schema/JsonSchemas.hpp>

namespace antwika::replay::detail
{

    using schema::countSchema;
    using schema::wordSchema;

    namespace
    {
        nlohmann::json buildTickEventSchema()
        {
            nlohmann::json eventSchema;
            eventSchema["type"] = "object";
            eventSchema["additionalProperties"] = false;
            eventSchema["required"] = {"name", "payload"}; // GCOVR_EXCL_LINE
            eventSchema["properties"]["name"] = wordSchema();
            eventSchema["properties"]["payload"] = wordSchema();

            nlohmann::json timedEvent;
            timedEvent["type"] = "object";
            timedEvent["additionalProperties"] = false;
            timedEvent["required"] = {"tick", "event"}; // GCOVR_EXCL_LINE
            timedEvent["properties"]["tick"] = countSchema();
            timedEvent["properties"]["event"] = eventSchema;
            return timedEvent;
        }
    }

    const nlohmann::json &tickEventSchema()
    {
        static const nlohmann::json shape =
            buildTickEventSchema(); // GCOVR_EXCL_LINE
        return shape;
    }

}
