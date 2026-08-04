#include "antwika/replay/JsonShapes.hpp"

#include <limits>
#include <string>

namespace antwika::replay
{

    // gcov puts each function's cleanup block on its closing brace.
    // Returning an nlohmann::json by value is what creates one.
    // No input reaches any of them: the functions are covered.
    // ReplayJson.cpp's own encoder explains it at length.

    nlohmann::json countShape()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = 0;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json boundedCountShape(std::int64_t maximum)
    {
        nlohmann::json shape = countShape();
        shape["maximum"] = maximum;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json coordinateShape()
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = std::numeric_limits<std::int32_t>::min();
        shape["maximum"] = std::numeric_limits<std::int32_t>::max();
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json wordShape()
    {
        nlohmann::json shape;
        shape["type"] = "string";
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json requiredShape(
        std::initializer_list<std::string_view> members)
    {
        auto shape = nlohmann::json::array();

        for (const auto member : members)
        {
            shape.push_back(std::string(member)); // GCOVR_EXCL_LINE
        }

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json objectShape(
        std::initializer_list<std::string_view> required)
    {
        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = requiredShape(required);
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json documentShape(
        std::string_view title,
        std::initializer_list<std::string_view> required)
    {
        // Draft 07 throughout, which is what every format here states.
        nlohmann::json shape = objectShape(required);
        shape["$schema"] = "http://json-schema.org/draft-07/schema#";
        shape["title"] = std::string(title);
        return shape;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
