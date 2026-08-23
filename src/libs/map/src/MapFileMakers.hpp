#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

#include "MapFileField.hpp"
#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

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

    template <auto Member>
    [[nodiscard]] constexpr Field cellField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &cellSchema,
            .valueOf = [](const void *record)
            { return jsonOf(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = voxelPositionFrom(json); }};
    }

    [[nodiscard]] inline nlohmann::json cellListShape()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"] = cellSchema();

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field cellListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &cellListShape,
            .valueOf = [](const void *record)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto cell : memberOf<Member>(record))
                {
                    arrayJson.push_back(jsonOf(cell));
                }

                return arrayJson;
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = readCells(json); }};
    }

    template <auto Member>
    [[nodiscard]] constexpr Field tintField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &colourSchema,
            .valueOf = [](const void *record)
            { return jsonOf(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = colorFrom(json); }};
    }

    template <auto Member>
    [[nodiscard]] constexpr Field tileField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &tileSchema,
            .valueOf = [](const void *record)
            { return writtenTile(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = readTile(json); }};
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
