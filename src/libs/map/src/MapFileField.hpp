#pragma once

#include <nlohmann/json.hpp>

#include <span>
#include <string>
#include <string_view>

namespace antwika::map::mapfile
{

    struct Field final
    {
        std::string_view key;

        nlohmann::json (*shape)();

        nlohmann::json (*valueOf)(const void *);

        void (*setFrom)(void *, const nlohmann::json &);
    };

    using Fields = std::span<const Field>;

    [[nodiscard]] nlohmann::json shapeOf(Fields fields);

    [[nodiscard]] nlohmann::json getWrittenObject(
        Fields fields, const void *record);

    void readObject(
        Fields fields, void *record, const nlohmann::json &json);

    template <typename Record>
    [[nodiscard]] nlohmann::json written(
        const Fields fields, const Record &record)
    {
        return getWrittenObject(fields, &record);
    } // GCOVR_EXCL_LINE

    template <typename Record>
    [[nodiscard]] Record read(
        const Fields fields, const nlohmann::json &json)
    {
        Record record;

        readObject(fields, &record, json);

        return record;
    } // GCOVR_EXCL_LINE

}
