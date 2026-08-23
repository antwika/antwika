#include "MapFileField.hpp"

#include "MapFileTables.hpp"

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

    nlohmann::json getWrittenObject(
        const Fields fields, const void *record)
    {
        nlohmann::json objectJson;

        for (const auto &field : fields)
        {
            objectJson[std::string(field.key)] = field.valueOf(record);
        }

        return objectJson;
    } // GCOVR_EXCL_LINE

    nlohmann::json getPlateSchema()
    {
        return shapeOf(kPlateFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getTransitionSchema()
    {
        return shapeOf(kTransitionFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getFlipSchema()
    {
        return shapeOf(kFlipFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getFamilySchema()
    {
        return shapeOf(kFamilyFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getCharacterSchemaLatest()
    {
        return shapeOf(kCharacterFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getDecorSchema()
    {
        return shapeOf(kDecorFields);
    } // GCOVR_EXCL_LINE

    nlohmann::json getMarkedCubeSchema()
    {
        return getOrNullShape(shapeOf(kMarkedCubeFields));
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
