#include "EventSchema.hpp"

#include <antwika/replay/JsonShapes.hpp>

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
            eventShape["properties"]["name"] = wordShape();
            eventShape["properties"]["payload"] = wordShape();

            nlohmann::json timedEvent;
            timedEvent["type"] = "object";
            timedEvent["additionalProperties"] = false;
            timedEvent["required"] = {"tick", "event"}; // GCOVR_EXCL_LINE
            timedEvent["properties"]["tick"] = countShape();
            timedEvent["properties"]["event"] = eventShape;
            return timedEvent;
        }
    }

    const nlohmann::json &tickEventShape()
    {
        static const nlohmann::json shape =
            buildTickEventShape(); // GCOVR_EXCL_LINE
        return shape;
    }

}
