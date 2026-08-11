#include "antwika/tileset/TilesetJson.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/enums/FromName.hpp>
#include <antwika/enums/NameTable.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/tileset/Sprite.hpp"
#include "antwika/tileset/Tileset.hpp"
#include "antwika/tileset/TilesetError.hpp"

namespace antwika::tileset
{

    namespace
    {
        using nlohmann::ordered_json;

        using tilemap::TerrainClass;

        constexpr int kIndent = 2;

        constexpr enums::NameTable<TerrainClass> kTerrainNames{
            {"floor", "wall", "water", "cliff", "path", "stair"}};

        constexpr std::array<std::string_view, 4> kSideKeys{
            "n", "e", "s", "w"};

        [[nodiscard]] std::string socketName(
            const Tileset &set, const SocketId socket)
        {
            if (socket >= set.socketNames.size())
            {
                throw TilesetError(
                    "the sprite names a socket outside the intern "
                    "table: "
                    + std::to_string(socket));
            }

            return set.socketNames[socket];
        }

        [[nodiscard]] ordered_json socketsJson(
            const Tileset &set, const Sprite &sprite)
        {
            ordered_json out;

            for (std::size_t side = 0; side < kSideKeys.size(); ++side)
            {
                out[std::string(kSideKeys[side])] =
                    socketName(set, sprite.sockets[side]);
            }

            return out;
        }

        [[nodiscard]] ordered_json spriteJson(
            const Tileset &set, const Sprite &sprite)
        {
            ordered_json out;
            out["id"] = sprite.id;

            if (sprite.frameCount != 1)
            {
                out["frames"] = sprite.frameCount;
            }

            if (sprite.weight != kDefaultWeight)
            {
                out["weight"] = sprite.weight;
            }

            out["sockets"] = socketsJson(set, sprite);
            out["on"] = sprite.on;

            return out;
        }

        [[nodiscard]] ordered_json layerJson(
            const Tileset &set,
            const Layer &layer,
            const std::size_t index)
        {
            ordered_json out;
            out["name"] = layer.name;

            if (index >= 1 && layer.density != kDefaultDensity)
            {
                out["density"] = layer.density;
            }

            auto sprites = ordered_json::array();

            for (const auto &sprite : layer.sprites)
            {
                sprites.push_back(spriteJson(set, sprite));
            }

            out["sprites"] = std::move(sprites);

            return out;
        }

        [[nodiscard]] const ordered_json &member(
            const ordered_json &object, const std::string &key)
        {
            if (!object.is_object())
            {
                throw TilesetError(
                    "the entry that should hold " + key
                    + " is not an object");
            }

            if (!object.contains(key))
            {
                throw TilesetError(
                    "the entry lacks the member: " + key);
            }

            return object.at(key);
        }

        [[nodiscard]] std::int64_t wholeValue(
            const ordered_json &value,
            const std::int64_t minimum,
            const std::int64_t maximum,
            const std::string &what)
        {
            if (!value.is_number_integer())
            {
                throw TilesetError(what + " is not an integer");
            }

            constexpr auto kTop =
                std::numeric_limits<std::int64_t>::max();

            if (value.is_number_unsigned()
                && value.get<std::uint64_t>()
                       > static_cast<std::uint64_t>(kTop))
            {
                throw TilesetError(what + " is out of range");
            }

            const auto number = value.get<std::int64_t>();

            if (number < minimum || number > maximum)
            {
                throw TilesetError(what + " is out of range");
            }

            return number;
        }

        [[nodiscard]] std::int64_t wholeMember(
            const ordered_json &object,
            const std::string &key,
            const std::int64_t minimum,
            const std::int64_t maximum)
        {
            return wholeValue(
                member(object, key), minimum, maximum,
                "the member " + key);
        }

        [[nodiscard]] std::string stringMember(
            const ordered_json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_string())
            {
                throw TilesetError(
                    "the member is not a string: " + key);
            }

            return value.get<std::string>();
        }

        void decodeSockets(
            Tileset &set, const ordered_json &entry, Sprite &sprite)
        {
            const std::string socketsKey = "sockets";
            const auto &sockets = member(entry, socketsKey);

            for (std::size_t side = 0; side < kSideKeys.size(); ++side)
            {
                const auto key = std::string(kSideKeys[side]);
                const auto name = stringMember(sockets, key);

                if (name.empty())
                {
                    throw TilesetError(
                        "the socket name is empty: " + key);
                }

                sprite.sockets[side] = internSocket(set, name);
            }
        }

        [[nodiscard]] std::vector<SpriteId> baseIdsOf(
            const Tileset &set)
        {
            std::vector<SpriteId> ids;
            ids.reserve(set.layers.front().sprites.size());

            for (const auto &sprite : set.layers.front().sprites)
            {
                ids.push_back(sprite.id);
            }

            return ids;
        } // GCOVR_EXCL_LINE

        void decodeBaseOn(const ordered_json &entry)
        {
            if (!entry.contains("on"))
            {
                return;
            }

            const auto &value = entry.at("on");

            if (!value.is_array())
            {
                throw TilesetError("the member is not an array: on");
            }

            if (!value.empty())
            {
                throw TilesetError(
                    "the base layer sprite carries an on list");
            }
        }

        void decodeDecorOn(
            const Tileset &set,
            const ordered_json &entry,
            Sprite &sprite)
        {
            if (!entry.contains("on"))
            {
                return;
            }

            const auto &value = entry.at("on");

            if (!value.is_array())
            {
                throw TilesetError("the member is not an array: on");
            }

            const auto baseIds = baseIdsOf(set);

            for (const auto &id : value)
            {
                const auto candidate =
                    static_cast<SpriteId>(wholeValue(
                        id, 0,
                        std::numeric_limits<std::uint32_t>::max(),
                        "the on id"));

                if (std::ranges::find(baseIds, candidate)
                    != baseIds.end())
                {
                    sprite.on.push_back(candidate);
                }
            }
        }

        [[nodiscard]] Sprite spriteFrom(
            Tileset &set,
            const ordered_json &entry,
            const std::size_t layerIndex)
        {
            Sprite sprite;
            sprite.id = static_cast<SpriteId>(wholeMember(
                entry, "id", 0,
                std::numeric_limits<std::uint32_t>::max()));

            if (entry.contains("frames"))
            {
                sprite.frameCount = static_cast<std::uint8_t>(
                    wholeMember(entry, "frames", 1, kMaxFrames));
            }

            if (entry.contains("weight"))
            {
                sprite.weight = static_cast<std::uint8_t>(
                    wholeMember(
                        entry, "weight", kMinWeight, kMaxWeight));
            }

            decodeSockets(set, entry, sprite);

            if (layerIndex == 0)
            {
                decodeBaseOn(entry);
            }
            else
            {
                decodeDecorOn(set, entry, sprite);
            }

            return sprite;
        }

        [[nodiscard]] Layer layerFrom(
            Tileset &set,
            const ordered_json &entry,
            const std::size_t layerIndex)
        {
            Layer layer;
            layer.name = stringMember(entry, "name");

            if (layerIndex >= 1 && entry.contains("density"))
            {
                layer.density = static_cast<std::uint8_t>(
                    wholeMember(entry, "density", 0, 255));
            }

            const std::string spritesKey = "sprites";
            const auto &sprites = member(entry, spritesKey);

            if (!sprites.is_array())
            {
                throw TilesetError(
                    "the member is not an array: sprites");
            }

            layer.sprites.reserve(sprites.size());

            for (const auto &sprite : sprites)
            {
                layer.sprites.push_back(
                    spriteFrom(set, sprite, layerIndex));
            }

            return layer;
        }

        void validateSpriteIds(const Tileset &set)
        {
            std::vector<SpriteId> ids;

            for (const auto &layer : set.layers)
            {
                for (const auto &sprite : layer.sprites)
                {
                    ids.push_back(sprite.id);
                }
            }

            if (ids.empty())
            {
                return;
            }

            std::ranges::sort(ids);

            if (std::ranges::adjacent_find(ids) != ids.end())
            {
                throw TilesetError(
                    "the tileset repeats a sprite id");
            }

            if (ids.back() >= set.nextSpriteId)
            {
                throw TilesetError(
                    "nextSpriteId does not clear the highest sprite "
                    "id");
            }
        }
    }

    std::string toJson(const Tileset &set)
    {
        ordered_json out;
        out["schema"] = kSchemaVersion;
        out["name"] = set.name;
        out["terrain"] = std::string(toString(set.terrain));
        out["nextSpriteId"] = set.nextSpriteId;

        auto layers = ordered_json::array();

        for (std::size_t index = 0; index < set.layers.size(); ++index)
        {
            layers.push_back(
                layerJson(set, set.layers[index], index));
        }

        out["layers"] = std::move(layers);

        return out.dump(kIndent);
    }

    Tileset tilesetFromJson(const std::string_view text)
    {
        ordered_json document;

        try
        {
            document = ordered_json::parse(text);
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw TilesetError(
                std::string(
                    "the tileset document is not valid json: ")
                + error.what());
        }

        if (!document.is_object())
        {
            throw TilesetError(
                "the tileset document is not an object");
        }

        const auto schema = wholeMember(
            document, "schema", 0,
            std::numeric_limits<std::int64_t>::max());

        if (schema != kSchemaVersion)
        {
            throw TilesetError(
                "the tileset names a schema version this build does "
                "not know: "
                + std::to_string(schema));
        }

        Tileset set;
        set.name = stringMember(document, "name");
        set.terrain = enums::fromName<TilesetError>(
            kTerrainNames,
            stringMember(document, "terrain"),
            "the tileset names an unknown terrain: ");
        set.nextSpriteId = static_cast<SpriteId>(wholeMember(
            document, "nextSpriteId", 0,
            std::numeric_limits<std::uint32_t>::max()));

        const std::string layersKey = "layers";
        const auto &layers = member(document, layersKey);

        if (!layers.is_array())
        {
            throw TilesetError(
                "the member is not an array: layers");
        }

        if (layers.empty())
        {
            throw TilesetError("the tileset holds no layers");
        }

        set.layers.clear();

        for (std::size_t index = 0; index < layers.size(); ++index)
        {
            set.layers.push_back(
                layerFrom(set, layers.at(index), index));
        }

        validateSpriteIds(set);

        return set;
    }

}
