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

    [[nodiscard]] inline nlohmann::json getFlagShape()
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
            .shape = &getFlagShape,
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
            .shape = &getCellSchema,
            .valueOf = [](const void *record)
            { return jsonOf(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = voxelPositionFrom(json); }};
    }

    [[nodiscard]] inline nlohmann::json getCellListShape()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"] = getCellSchema();

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field cellListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getCellListShape,
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
            { memberIn<Member>(record) = getReadCells(json); }};
    }

    template <auto Member>
    [[nodiscard]] constexpr Field tintField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getColourSchema,
            .valueOf = [](const void *record)
            { return jsonOf(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = colorFrom(json); }};
    }

    [[nodiscard]] inline nlohmann::json getTextShape()
    {
        nlohmann::json shape;

        shape["type"] = "string";

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field textField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getTextShape,
            .valueOf = [](const void *record)
            { return nlohmann::json(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = json.get<std::string>(); }};
    }

    template <auto Member>
    [[nodiscard]] constexpr Field tileField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getTileSchema,
            .valueOf = [](const void *record)
            { return getWrittenTile(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            { memberIn<Member>(record) = getReadTile(json); }};
    }

    [[nodiscard]] inline nlohmann::json getTextListShape()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"]["type"] = "string";

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field textListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getTextListShape,
            .valueOf = [](const void *record)
            { return nlohmann::json(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                auto &lines = memberIn<Member>(record);

                lines.clear();

                for (const auto &lineJson : json)
                {
                    lines.push_back(lineJson.get<std::string>());
                }
            }};
    }

    [[nodiscard]] inline nlohmann::json getFixedPlaceShape()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"] =
            getWholeSchema(-kMaxCameraCoord, kMaxCameraCoord);
        shape["minItems"] = kAxisCount;
        shape["maxItems"] = kAxisCount;

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field fixedPlaceField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getFixedPlaceShape,
            .valueOf = [](const void *record)
            {
                const auto place = memberOf<Member>(record);

                return nlohmann::json::array(
                    {toFixed(place.x),
                     toFixed(place.y),
                     toFixed(place.z)});
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Member>(record) = gfx::Vec3{
                    getFromFixed(json[0].get<std::int64_t>()),
                    getFromFixed(json[1].get<std::int64_t>()),
                    getFromFixed(json[2].get<std::int64_t>())};
            }};
    }

    [[nodiscard]] inline nlohmann::json getUniqueTileListShape()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"] = getTileSchema();
        shape["uniqueItems"] = true;

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field uniqueTileListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getUniqueTileListShape,
            .valueOf = [](const void *record)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto tile : memberOf<Member>(record))
                {
                    arrayJson.push_back(getWrittenTile(tile));
                }

                return arrayJson;
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                auto &tiles = memberIn<Member>(record);

                tiles.clear();

                for (const auto &tileJson : json)
                {
                    tiles.push_back(getReadTile(tileJson));
                }
            }};
    }

    template <auto First, auto Second, int Least, int Most>
    [[nodiscard]] constexpr Field pairField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = []
            {
                nlohmann::json shape;

                shape["type"] = "array";
                shape["items"] = getWholeSchema(Least, Most);
                shape["minItems"] = 2;
                shape["maxItems"] = 2;

                return shape;
            },
            .valueOf = [](const void *record)
            {
                return nlohmann::json::array(
                    {memberOf<First>(record),
                     memberOf<Second>(record)});
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<First>(record) =
                    json[0].get<Held<First>>();
                memberIn<Second>(record) =
                    json[1].get<Held<Second>>();
            }};
    }

    [[nodiscard]] inline nlohmann::json getMaybeTileListShape()
    {
        nlohmann::json nullShape;

        nullShape["type"] = "null";

        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"]["oneOf"] = {getTileSchema(), nullShape};

        return shape;
    } // GCOVR_EXCL_LINE

    template <auto Member>
    [[nodiscard]] constexpr Field maybeTileListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getMaybeTileListShape,
            .valueOf = [](const void *record)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto tile : memberOf<Member>(record))
                {
                    arrayJson.push_back(
                        tile.has_value() ? getWrittenTile(*tile)
                                         : nlohmann::json());
                }

                return arrayJson;
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                auto &tiles = memberIn<Member>(record);

                tiles.clear();

                for (const auto &tileJson : json)
                {
                    tiles.push_back(
                        tileJson.is_null()
                            ? std::optional<tilemap::Tile>{}
                            : std::optional<tilemap::Tile>{
                                  getReadTile(tileJson)});
                }
            }};
    }

    template <auto Outer, auto Inner>
    [[nodiscard]] constexpr Field nestedPlaceField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = &getFixedPlaceShape,
            .valueOf = [](const void *record)
            {
                const auto place = memberOf<Outer>(record).*Inner;

                return nlohmann::json::array(
                    {toFixed(place.x),
                     toFixed(place.y),
                     toFixed(place.z)});
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Outer>(record).*Inner = gfx::Vec3{
                    getFromFixed(json[0].get<std::int64_t>()),
                    getFromFixed(json[1].get<std::int64_t>()),
                    getFromFixed(json[2].get<std::int64_t>())};
            }};
    }

    template <auto Outer, auto Inner>
    [[nodiscard]] constexpr Field nestedFixedField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = []
            {
                return getWholeSchema(
                    -kMaxCameraCoord, kMaxCameraCoord);
            },
            .valueOf = [](const void *record)
            {
                return nlohmann::json(
                    toFixed(memberOf<Outer>(record).*Inner));
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Outer>(record).*Inner =
                    getFromFixed(json.get<std::int64_t>());
            }};
    }

    [[nodiscard]] inline nlohmann::json getOrNullShape(
        nlohmann::json shape)
    {
        nlohmann::json nullShape;

        nullShape["type"] = "null";

        nlohmann::json either;

        either["oneOf"] = {std::move(shape), nullShape};

        return either;
    } // GCOVR_EXCL_LINE

    template <auto Member, const auto &Table>
    [[nodiscard]] constexpr Field recordField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = [] { return shapeOf(Table); },
            .valueOf = [](const void *record)
            { return written(Table, memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Member>(record) =
                    read<Held<Member>>(Table, json);
            }};
    }

    template <auto Member, int Least, int Most>
    [[nodiscard]] constexpr Field wholeField(const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = [] { return getWholeSchema(Least, Most); },
            .valueOf = [](const void *record)
            { return nlohmann::json(memberOf<Member>(record)); },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Member>(record) =
                    json.get<Held<Member>>();
            }};
    }

    template <auto Member, const auto &Table, std::size_t Least>
    [[nodiscard]] constexpr Field recordListField(
        const std::string_view key)
    {
        using Kept = typename Held<Member>::value_type;

        return Field{
            .key = key,
            .shape = []
            {
                nlohmann::json shape;

                shape["type"] = "array";
                shape["items"] = shapeOf(Table);
                shape["minItems"] = Least;

                return shape;
            },
            .valueOf = [](const void *record)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &heldRecord : memberOf<Member>(record))
                {
                    arrayJson.push_back(written(Table, heldRecord));
                }

                return arrayJson;
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                auto &heldRecords = memberIn<Member>(record);

                heldRecords.clear();

                for (const auto &heldJson : json)
                {
                    heldRecords.push_back(read<Kept>(Table, heldJson));
                }
            }};
    }

    template <auto Member, std::size_t Least, std::size_t Most>
    [[nodiscard]] constexpr Field tileListField(
        const std::string_view key)
    {
        return Field{
            .key = key,
            .shape = []
            {
                nlohmann::json shape;

                shape["type"] = "array";
                shape["items"] = getTileSchema();
                shape["minItems"] = Least;
                shape["maxItems"] = Most;

                return shape;
            },
            .valueOf = [](const void *record)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto tile : memberOf<Member>(record))
                {
                    arrayJson.push_back(getWrittenTile(tile));
                }

                return arrayJson;
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                auto &tiles = memberIn<Member>(record);

                tiles.clear();

                for (const auto &tileJson : json)
                {
                    tiles.push_back(getReadTile(tileJson));
                }
            }};
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
                    std::string(Names.getName(memberOf<Member>(record))));
            },
            .setFrom = [](void *record, const nlohmann::json &json)
            {
                memberIn<Member>(record) =
                    enumFromName(Names, json.get<std::string>());
            }};
    }

}
