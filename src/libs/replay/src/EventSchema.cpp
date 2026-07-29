#include "EventSchema.hpp"

namespace antwika::replay::detail
{

    namespace
    {
        nlohmann::json buildTickEventShape()
        {
            nlohmann::json eventShape;
            eventShape["type"] = "object";
            eventShape["additionalProperties"] = false;
            eventShape["required"] = {"name", "payload"}; // GCOVR_EXCL_LINE
            eventShape["properties"]["name"]["type"] = "string";
            eventShape["properties"]["payload"]["type"] = "string";

            nlohmann::json timedEvent;
            timedEvent["type"] = "object";
            timedEvent["additionalProperties"] = false;
            timedEvent["required"] = {"tick", "event"}; // GCOVR_EXCL_LINE
            timedEvent["properties"]["tick"]["type"] = "integer";
            timedEvent["properties"]["tick"]["minimum"] = 0;
            timedEvent["properties"]["event"] = eventShape;
            return timedEvent;
        }
    } // namespace

    const nlohmann::json &timedEventShape()
    {
        static const nlohmann::json shape =
            buildTickEventShape(); // GCOVR_EXCL_LINE
        return shape;
    }

} // namespace antwika::replay::detail
