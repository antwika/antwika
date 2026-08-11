#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/tileset/Sprite.hpp"

namespace antwika::tileset
{

    inline constexpr std::uint8_t kDefaultDensity = 64;

    struct Layer final
    {
        std::string name{};

        /**
         * @brief The decor scatter threshold, 0 to 255.
         *
         * Requires: read only on layers at index 1 and above.
         */
        std::uint8_t density = kDefaultDensity;

        std::vector<Sprite> sprites{};

        [[nodiscard]] bool operator==(
            const Layer &other) const = default;
    };

    struct Tileset final
    {
        std::string name{};
        tilemap::TerrainClass terrain = tilemap::TerrainClass::Floor;
        SpriteId nextSpriteId = 0;

        /**
         * @brief The socket intern table a SocketId indexes.
         *
         * Requires: indices 0 and 1 stay "edge" and "open".
         */
        std::vector<std::string> socketNames{"edge", "open"};

        std::vector<Layer> layers{Layer{.name = "base"}};

        [[nodiscard]] bool operator==(
            const Tileset &other) const = default;
    };

    /**
     * @brief Finds or appends a socket name in the intern table.
     *
     * @param set The tileset whose table grows.
     * @param name The socket name to intern.
     * @return The index the name holds in socketNames.
     *
     * Ensures: a name already in the table keeps its index.
     */
    [[nodiscard]] SocketId internSocket(
        Tileset &set, std::string_view name);

    /**
     * @brief Appends a blank sprite to a layer.
     *
     * @param set The tileset that allocates the id.
     * @param layer The index of the layer to grow.
     * @return The appended sprite.
     * @throws TilesetError If the layer index names no layer.
     *
     * Ensures: the sprite takes nextSpriteId and nextSpriteId moves
     *          past it, so ids never repeat.
     */
    Sprite &addSprite(Tileset &set, std::size_t layer);

    /**
     * @brief Erases a sprite from a layer.
     *
     * @param set The tileset to shrink.
     * @param layer The index of the layer to shrink.
     * @param index The position of the sprite in that layer.
     * @throws TilesetError If the layer index names no layer or the
     *                      position names no sprite.
     *
     * Ensures: a removed base sprite's id leaves every decor layer's
     *          on list, and nextSpriteId stays put.
     */
    void removeSprite(
        Tileset &set, std::size_t layer, std::size_t index);

    Layer &addLayer(Tileset &set, std::string name);

    /**
     * @brief Erases a decor layer.
     *
     * @param set The tileset to shrink.
     * @param index The index of the layer to erase.
     * @throws TilesetError If the index is 0, because the base layer
     *                      stays, or if it names no layer.
     */
    void removeLayer(Tileset &set, std::size_t index);

}
