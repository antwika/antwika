#include <nlohmann/json.hpp>

#include <span>
#include <string>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map::mapfile
{

    namespace
    {
        [[nodiscard]] nlohmann::json namesOf(
            const std::span<const std::string_view> names)
        {
            auto arrayJson = nlohmann::json::array();

            for (const auto name : names)
            {
                arrayJson.push_back(std::string(name));
            }

            return arrayJson;
        } // GCOVR_EXCL_LINE
    }

    nlohmann::json settingsSchema()
    {
        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = {
            std::string(kLightingKey),
            std::string(kTiesKey),
            std::string(kToolKey),
            std::string(kDrawingKey),
            std::string(kViewKey),
            std::string(kKindKey),
            std::string(kGridKey),
            std::string(kMarkerKey),
            std::string(kSightKey),
            std::string(kFollowingKey),
            std::string(kAboveHiddenKey),
            std::string(kCornersJoinedKey)};
        shape["properties"][std::string(kLightingKey)]["type"] =
            "boolean";
        shape["properties"][std::string(kTiesKey)]["type"] =
            "boolean";
        shape["properties"][std::string(kToolKey)]["enum"] =
            namesOf(kToolNames);
        shape["properties"][std::string(kDrawingKey)]["enum"] =
            namesOf(kDrawingNames);
        shape["properties"][std::string(kViewKey)]["enum"] =
            namesOf(kViewNames);
        shape["properties"][std::string(kKindKey)]["enum"] =
            namesOf(kKindNames);

        for (const auto key :
             {kGridKey,
              kMarkerKey,
              kSightKey,
              kFollowingKey,
              kAboveHiddenKey,
              kCornersJoinedKey})
        {
            shape["properties"][std::string(key)]["type"] =
                "boolean";
        }

        return shape;
    } // GCOVR_EXCL_LINE

    void gatesSchemaWiring(nlohmann::json &schema)
    {
        for (const auto key :
             {kKeysKey,
              kDoorsKey,
              kCheckpointsKey,
              kFoodKey,
              kWaterKey})
        {
            schema["required"].push_back(std::string(key));
            schema["properties"][std::string(key)]["type"] =
                "array";
            schema["properties"][std::string(key)]["items"] =
                cellSchema();
        }

        schema["required"].push_back(
            std::string(kExitLockedKey));
        schema["properties"][std::string(kExitLockedKey)]
              ["type"] = "boolean";
    }

}
