#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

#include "MapFileField.hpp"
#include "MapFileShared.hpp"

namespace antwika::map::mapfile
{

    template <typename Record, typename Value>
    Record holderOf(Value Record::*);

    template <typename Record, typename Value>
    Value heldBy(Value Record::*);

    template <auto Member>
    using Holds = decltype(holderOf(Member));

    template <auto Member>
    using Held = decltype(heldBy(Member));

    template <auto Member>
    [[nodiscard]] const Held<Member> &memberOf(const void *record)
    {
        return static_cast<const Holds<Member> *>(record)->*Member;
    }

    template <auto Member>
    [[nodiscard]] Held<Member> &memberIn(void *record)
    {
        return static_cast<Holds<Member> *>(record)->*Member;
    }

    [[nodiscard]] inline nlohmann::json flagShape()
    {
        nlohmann::json shape;

        shape["type"] = "boolean";

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field flagField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &flagShape,
            .valueOf = [](const void *record)
            { return nlohmann::json(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = json.get<bool>(); }};
    }

    template <auto Member, const auto &Names>
    [[nodiscard]] constexpr Field namedField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = []
            {
                nlohmann::json shape;

                shape["enum"] = namesOf(Names.names);

                return shape;
            },
            .valueOf = [](const void *record)
            {
                return nlohmann::json(
                    std::string(Names.name(memberOf<Member>(record))));
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Member>(record) =
                    enumFromName(Names, json.get<std::string>());
            }};
    }

}
