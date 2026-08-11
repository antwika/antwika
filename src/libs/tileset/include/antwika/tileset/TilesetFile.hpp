#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>

#include "antwika/tileset/Tileset.hpp"

namespace antwika::tileset
{

    /**
     * @brief Renders a layer's sprites into a class-coded bitmap.
     *
     * @param layer The layer to render.
     * @return A kAtlasWidth wide bitmap, one sprite row per sprite.
     *
     * Ensures: ink pixels come out opaque white, paper pixels opaque
     *          mid-gray, and blank pixels and frame slots past a
     *          sprite's frameCount fully transparent.
     */
    [[nodiscard]] gfx::Bitmap layerBitmapOf(const Layer &layer);

    /**
     * @brief Reads class-coded pixels back into a layer's sprites.
     *
     * @param layer The layer whose sprites take the pixels.
     * @param bitmap The decoded layer image.
     * @throws TilesetError If the bitmap is not kAtlasWidth wide and
     *                      one sprite row tall per sprite.
     *
     * Ensures: opaque pixels normalize to ink at luminance 192 and
     *          above and to paper below it, transparent pixels to
     *          blank, and frame slots past a sprite's frameCount are
     *          ignored.
     */
    void readLayerBitmap(Layer &layer, const gfx::Bitmap &bitmap);

    /**
     * @brief Writes a tileset directory: tileset.json beside one
     *        layer image per non-empty layer.
     *
     * @param directory The directory to write, created if missing.
     * @param set The tileset to write into it.
     * @throws TilesetError If the directory or a file cannot be
     *                      written.
     *
     * Ensures: a layer without sprites leaves no layer image behind.
     */
    void saveTileset(
        const std::filesystem::path &directory, const Tileset &set);

    /**
     * @brief Reads a tileset directory.
     *
     * @param directory The directory to read.
     * @return The tileset it holds.
     * @throws TilesetError If tileset.json cannot be read or fails
     *                      validation, or a non-empty layer's image
     *                      is missing, unreadable, or the wrong
     *                      shape.
     *
     * Ensures: a layer that declares no sprites loads without a
     *          layer image.
     */
    [[nodiscard]] Tileset loadTileset(
        const std::filesystem::path &directory);

    /**
     * @brief Names the tileset directories under an assets
     *        directory.
     *
     * @param assetsDir The directory to scan.
     * @return The subdirectories holding a tileset.json, sorted.
     *
     * Ensures: a missing assets directory lists nothing.
     */
    [[nodiscard]] std::vector<std::string> listTilesets(
        const std::filesystem::path &assetsDir);

    /**
     * @brief Loads every tileset under an assets directory.
     *
     * @param assetsDir The directory to scan.
     * @return The tilesets that load, sorted by tileset name.
     *
     * Ensures: a directory whose tileset fails to load is skipped
     *          rather than failing the rest.
     */
    [[nodiscard]] std::vector<Tileset> loadTilesetLibrary(
        const std::filesystem::path &assetsDir);

}
