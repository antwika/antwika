#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace antwika::replay
{

    [[nodiscard]] nlohmann::json countShape();

    [[nodiscard]] nlohmann::json boundedCountShape(std::int64_t maximum);

    [[nodiscard]] nlohmann::json coordinateShape();

    [[nodiscard]] nlohmann::json wordShape();

    [[nodiscard]] nlohmann::json requiredShape(
        std::initializer_list<std::string_view> members);

    [[nodiscard]] nlohmann::json objectShape(
        std::initializer_list<std::string_view> required);

    [[nodiscard]] nlohmann::json documentShape(
        std::string_view title,
        std::initializer_list<std::string_view> required);

    template <auto BuildSchema>
    [[nodiscard]] const nlohmann::json_schema::json_validator &
    validatorFor()
    {
        // GCOVR_EXCL_START
        static const nlohmann::json_schema::json_validator validator(
            BuildSchema());
        // GCOVR_EXCL_STOP
        return validator;
    }

}
