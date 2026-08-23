#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace antwika::schema
{

    [[nodiscard]] nlohmann::json getCountSchema();

    [[nodiscard]] nlohmann::json getBoundedCountSchema(std::int64_t maximum);

    [[nodiscard]] nlohmann::json getCoordinateSchema();

    [[nodiscard]] nlohmann::json getWordSchema();

    [[nodiscard]] nlohmann::json getRequiredSchema(
        std::initializer_list<std::string_view> members);

    [[nodiscard]] nlohmann::json getObjectSchema(
        std::initializer_list<std::string_view> requiredKeys);

    [[nodiscard]] nlohmann::json getDocumentSchema(
        std::string_view title,
        std::initializer_list<std::string_view> requiredKeys);

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
