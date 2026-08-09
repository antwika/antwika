#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "antwika/enums/NameTable.hpp"

namespace antwika::enums
{

    /**
     * @brief Unwraps a lookup, complaining about the name that failed.
     *
     * @param found The result of the lookup.
     * @param complaint Text placed before the name in the message.
     * @param name The name that was looked up.
     * @return The value the lookup found.
     * @throws Error If the lookup found nothing.
     */
    template <typename Error, typename Value>
    [[nodiscard]] Value orThrow(
        const std::optional<Value> &found,
        const std::string_view complaint,
        const std::string_view name)
    {
        if (!found.has_value())
        {
            throw Error(std::string(complaint) + std::string(name));
        }

        return *found;
    }

    /**
     * @brief Reads an enumerator from its name.
     *
     * @param table The names this build knows.
     * @param name The name to read.
     * @param complaint Text placed before the name in the message.
     * @return The enumerator the table names.
     * @throws Error If the table holds no such name.
     */
    template <typename Error, typename Enum>
    [[nodiscard]] Enum fromName(
        const NameTable<Enum> &table,
        const std::string_view name,
        const std::string_view complaint)
    {
        return orThrow<Error>(table.from(name), complaint, name);
    }

}
