#pragma once

#include <array>
#include <string>
#include <string_view>

#include <antwika/atlas/AtlasMeta.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Bitmap.hpp>

#include "antwika/game/AtlasAssets.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    struct AtlasSheets final
    {
        std::array<antwika::gfx::Bitmap, kAtlasKindCount> images{};

        antwika::gfx::Bitmap walker{};

        AtlasSpecs specs{};

        [[nodiscard]] const antwika::gfx::Bitmap &of(
            AtlasKind kind) const noexcept
        {
            return antwika::enums::pick(images, kind);
        }
    };

    /**
     * @brief Reads every sheet the game draws from, and its metadata.
     *
     * @param assets The sheet to read for each kind, in AtlasKind
     *               order, and the sheet the walkers march on, named
     *               relative to the running executable.
     * @return The images, and the geometry their sidecars record.
     * @throws GfxError If a sheet or its sidecar is missing, if an
     *         image is not the size its sidecar records, or if the
     *         sheets cannot hold what the game draws.
     */
    [[nodiscard]] AtlasSheets loadAtlasSheets(const AtlasAssets &assets);

    /**
     * @brief Reads the metadata recorded beside a shipped sheet.
     *
     * @param name The sheet's asset name.
     * @return The metadata its sidecar holds.
     * @throws GfxError If the sheet ships without a sidecar.
     */
    [[nodiscard]] antwika::atlas::AtlasMeta atlasMetaAsset(
        std::string_view name);

    [[nodiscard]] AtlasSpec specFrom(
        const antwika::atlas::AtlasMeta &meta) noexcept;

    [[nodiscard]] AtlasSpecs specsFrom(
        const std::array<antwika::atlas::AtlasMeta, kAtlasKindCount>
            &metas,
        const antwika::atlas::AtlasMeta &walker) noexcept;

    /**
     * @brief Checks that an image is the size its metadata records.
     *
     * @param bitmap The image read from disk.
     * @param spec The geometry its sidecar records.
     * @param name The sheet's asset name, for the message.
     * @throws GfxError If the image is another size.
     */
    void requireAtlasSize(
        const antwika::gfx::Bitmap &bitmap,
        const AtlasSpec &spec,
        std::string_view name);

    /**
     * @brief Checks that the sheets can hold what the game draws.
     *
     * @param specs The geometry read from the sheets' metadata.
     * @throws GfxError If a sheet holds no slots, if the walker sheet
     *         has no room for a walk cycle in every facing, if a
     *         sheet has no slot for a sprite the game draws, if a
     *         footprint is not its whole number of tiles, or if a
     *         metric does not scale exactly at every zoom.
     */
    void requireAtlasSpecs(const AtlasSpecs &specs);

}
