#include "antwika/replay/JsonShapes.hpp"

#include <limits>

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

} // namespace antwika::replay
