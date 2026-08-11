#include "antwika/tileset/Tileset.hpp"

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "antwika/tileset/Sprite.hpp"
#include "antwika/tileset/TilesetError.hpp"

namespace antwika::tileset
{

    namespace
    {
        [[nodiscard]] Layer &layerAt(
            Tileset &set, const std::size_t layer)
        {
            if (layer >= set.layers.size())
            {
                throw TilesetError(
                    "the tileset holds no layer "
                    + std::to_string(layer));
            }

            return set.layers[layer];
        }
    }

    SocketId internSocket(Tileset &set, const std::string_view name)
    {
        for (std::size_t index = 0;
             index < set.socketNames.size();
             ++index)
        {
            if (set.socketNames[index] == name)
            {
                return static_cast<SocketId>(index);
            }
        }

        set.socketNames.emplace_back(name);

        return static_cast<SocketId>(set.socketNames.size() - 1);
    }

    Sprite &addSprite(Tileset &set, const std::size_t layer)
    {
        auto &sprites = layerAt(set, layer).sprites;

        sprites.push_back(Sprite{.id = set.nextSpriteId});
        ++set.nextSpriteId;

        return sprites.back();
    }

    void removeSprite(
        Tileset &set,
        const std::size_t layer,
        const std::size_t index)
    {
        auto &sprites = layerAt(set, layer).sprites;

        if (index >= sprites.size())
        {
            throw TilesetError(
                "the layer holds no sprite " + std::to_string(index));
        }

        const auto removed = sprites[index].id;

        sprites.erase(
            std::next(
                sprites.begin(),
                static_cast<std::ptrdiff_t>(index)));

        if (layer != 0)
        {
            return;
        }

        for (std::size_t decor = 1; decor < set.layers.size(); ++decor)
        {
            for (auto &sprite : set.layers[decor].sprites)
            {
                std::erase(sprite.on, removed);
            }
        }
    }

    Layer &addLayer(Tileset &set, std::string name)
    {
        Layer made;
        made.name = std::move(name);
        set.layers.push_back(std::move(made));

        return set.layers.back();
    }

    void removeLayer(Tileset &set, const std::size_t index)
    {
        if (index == 0)
        {
            throw TilesetError("the base layer cannot be removed");
        }

        if (index >= set.layers.size())
        {
            throw TilesetError(
                "the tileset holds no layer " + std::to_string(index));
        }

        set.layers.erase(
            std::next(
                set.layers.begin(),
                static_cast<std::ptrdiff_t>(index)));
    }

}
