#include "antwika/schema/JsonSchemas.hpp"

#include <limits>
#include <string>

namespace antwika::schema
{

    nlohmann::json getCountSchema()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = 0;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getBoundedCountSchema(std::int64_t maximum)
    {
        nlohmann::json shape = getCountSchema();
        shape["maximum"] = maximum;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getCoordinateSchema()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = std::numeric_limits<std::int32_t>::min();
        shape["maximum"] = std::numeric_limits<std::int32_t>::max();
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getWordSchema()
    {
        nlohmann::json shape;
        shape["type"] = "string";
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getRequiredSchema(
        std::initializer_list<std::string_view> members)
    {
        auto shape = nlohmann::json::array();

        for (const auto member : members)
        {
            shape.push_back(std::string(member)); // GCOVR_EXCL_LINE
        }

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getObjectSchema(
        std::initializer_list<std::string_view> requiredKeys)
    {
        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = getRequiredSchema(requiredKeys);
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getDocumentSchema(
        std::string_view title,
        std::initializer_list<std::string_view> requiredKeys)
    {
        nlohmann::json shape = getObjectSchema(requiredKeys);
        shape["$schema"] = "http://json-schema.org/draft-07/schema#";
        shape["title"] = std::string(title);
        return shape;
    } // GCOVR_EXCL_LINE

}
