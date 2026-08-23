#include "MapFileField.hpp"

namespace antwika::map::mapfile
{

    nlohmann::json shapeOf(const Fields fields)
    {
        nlohmann::json shape;

        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = nlohmann::json::array();

        for (const auto &field : fields)
        {
            const auto name = std::string(field.key);

            shape["required"].push_back(name);
            shape["properties"][name] = field.shape();
        }

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json writtenObject(
        const Fields fields, const void *record)
    {
        nlohmann::json objectJson;

        for (const auto &field : fields)
        {
            objectJson[std::string(field.key)] = field.valueOf(record);
        }

        return objectJson;
    } // GCOVR_EXCL_LINE

    void readObject(
        const Fields fields,
        void *record,
        const nlohmann::json &json)
    {
        for (const auto &field : fields)
        {
            field.setFrom(record, json.at(std::string(field.key)));
        }
    }

}
