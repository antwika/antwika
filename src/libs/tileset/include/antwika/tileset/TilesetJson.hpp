#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "antwika/tileset/Tileset.hpp"

namespace antwika::tileset
{

    inline constexpr std::uint32_t kSchemaVersion = 1;

    /**
     * @brief Serializes a tileset, leaving pixel data to the layer
     *        images.
     *
     * @param set The tileset to serialize.
     * @return The tileset document, indented, without a trailing
     *         newline.
     * @throws TilesetError If a sprite names a socket outside the
     *                      intern table.
     *
     * Ensures: members keep one fixed order, frames is omitted at 1,
     *          weight at kDefaultWeight and density at its default,
     *          sockets and on are always present, and equal tilesets
     *          serialize to equal bytes.
     */
    [[nodiscard]] std::string toJson(const Tileset &set);

    /**
     * @brief Reads a tileset out of a tileset document.
     *
     * @param text The document to parse.
     * @return The tileset it describes.
     * @throws TilesetError If the text is not valid json, fails
     *                      validation, or names a schema version or
     *                      terrain this build does not know.
     *
     * Ensures: socket names intern first-seen after the two reserved
     *          entries, and on ids that name no base sprite drop out.
     */
    [[nodiscard]] Tileset tilesetFromJson(std::string_view text);

}
