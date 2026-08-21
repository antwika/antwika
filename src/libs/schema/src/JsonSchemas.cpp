#include "antwika/schema/JsonSchemas.hpp"

#include <limits>
#include <string>

namespace antwika::schema
{

    nlohmann::json countSchema()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = 0;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json boundedCountSchema(std::int64_t maximum)
    {
        nlohmann::json shape = countSchema();
        shape["maximum"] = maximum;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json coordinateSchema()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = std::numeric_limits<std::int32_t>::min();
        shape["maximum"] = std::numeric_limits<std::int32_t>::max();
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json wordSchema()
    {
        nlohmann::json shape;
        shape["type"] = "string";
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json requiredSchema(
        std::initializer_list<std::string_view> members)
    {
        auto shape = nlohmann::json::array();

        for (const auto member : members)
        {
            shape.push_back(std::string(member)); // GCOVR_EXCL_LINE
        }

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json objectSchema(
        std::initializer_list<std::string_view> requiredKeys)
    {
        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = requiredSchema(requiredKeys);
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json documentSchema(
        std::string_view title,
        std::initializer_list<std::string_view> requiredKeys)
    {
        nlohmann::json shape = objectSchema(requiredKeys);
        shape["$schema"] = "http://json-schema.org/draft-07/schema#";
        shape["title"] = std::string(title);
        return shape;
    } // GCOVR_EXCL_LINE

}
