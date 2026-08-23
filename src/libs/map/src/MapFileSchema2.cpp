#include <nlohmann/json.hpp>

#include <span>
#include <string>

#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"
#include "MapFileTables.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map::mapfile
{

    nlohmann::json settingsSchema()
    {
        return shapeOf(kSettingsFields);
    } // GCOVR_EXCL_LINE

    void gatesSchemaWiring(nlohmann::json &schema)
    {
        for (const auto &row : kGateRows)
        {
            schema["required"].push_back(std::string(row.key));
            schema["properties"][std::string(row.key)]["type"] =
                "array";
            schema["properties"][std::string(row.key)]["items"] =
                cellSchema();
        }

        schema["required"].push_back(
            std::string(kExitLockedKey));
        schema["properties"][std::string(kExitLockedKey)]
              ["type"] = "boolean";
    }

}
