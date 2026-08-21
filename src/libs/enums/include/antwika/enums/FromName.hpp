#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "antwika/enums/NameTable.hpp"

namespace antwika::enums
{

    template <typename Error, typename Value>
    [[nodiscard]] Value orThrow(
        const std::optional<Value> &maybeValue,
        const std::string_view messagePrefix,
        const std::string_view name)
    {
        if (!maybeValue.has_value())
        {
            throw Error(std::string(messagePrefix) + std::string(name));
        }

        return *maybeValue;
    }

    template <typename Error, typename Enum>
    [[nodiscard]] Enum fromName(
        const NameTable<Enum> &table,
        const std::string_view name,
        const std::string_view messagePrefix)
    {
        return orThrow<Error>(table.from(name), messagePrefix, name);
    }

}
