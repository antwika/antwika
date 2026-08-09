#include "antwika/game/AtlasSheets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/atlas/AtlasMeta.hpp>
#include <antwika/atlas/AtlasMetaFile.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    using antwika::gfx::GfxError;

    namespace
    {
        constexpr std::string_view kWalkerSheet = "walker";

        [[nodiscard]] std::string named(const AtlasKind kind)
        {
            const std::array<std::string_view, kAtlasKindCount> sheets{
                "1x1", "2x2", "3x3"};

            return std::string(
                sheets[atlasKindIndex(kind) % kAtlasKindCount]);
        }

        [[nodiscard]] std::string describe(const Size &size)
        {
            return std::to_string(size.width) + "x"
                + std::to_string(size.height);
        }

        [[nodiscard]] std::uint32_t cellsOf(const AtlasKind kind) noexcept
        {
            return static_cast<std::uint32_t>(
                       atlasKindIndex(kind) % kAtlasKindCount)
                   + 1;
        }

        void requireSlots(
            const AtlasSpec &spec, const std::string_view name)
        {
            if (spec.slots() > 0)
            {
                return;
            }

            throw GfxError(
                "the " + std::string(name)
                + " atlas records no slots to draw from");
        }

        void requireWalkRoom(const AtlasSpec &spec)
        {
            if (spec.rows >= kWalkerSheetRows
                && spec.columns >= kWalkCycleFrames)
            {
                return;
            }

            throw GfxError(
                "the walker atlas has no room for a walk cycle in "
                "every facing");
        }

        void requireSprites(const AtlasSpec &spec, const AtlasKind kind)
        {
            const auto slot = atlasKindIndex(kind) % kAtlasKindCount;

            for (std::size_t index = 0; index < kBuildingKindCount;
                 ++index)
            {
                const auto building =
                    static_cast<BuildingKind>(index);

                if (buildingAtlasOf(building) == kind
                    && kBuildingSprites[index] >= spec.slots())
                {
                    throw GfxError(
                        "the " + named(kind)
                        + " atlas has no slot for a sprite the game "
                          "draws");
                }
            }

            if (kDebrisSprites[slot] >= spec.slots()
                || kFireSprites[slot] >= spec.slots())
            {
                throw GfxError(
                    "the " + named(kind)
                    + " atlas has no slot for its ruin sprites");
            }
        }

        void requireFootprint(
            const AtlasSpec &spec,
            const std::uint32_t cells,
            const std::string_view name)
        {
            if (spec.isometric.width == cells * kIsoTileSize.width
                && spec.isometric.height == cells * kIsoTileSize.height)
            {
                return;
            }

            throw GfxError(
                "the " + std::string(name)
                + " atlas records a footprint that is not "
                + std::to_string(cells) + " tiles on a side");
        }

        [[nodiscard]] bool halvesExactly(
            const std::int32_t value) noexcept
        {
            for (const auto halfWidth : kZoomHalfWidths)
            {
                if ((value * static_cast<std::int32_t>(halfWidth))
                        % kBaseHalfWidth
                    != 0)
                {
                    return false;
                }
            }

            return true;
        }

        void requireScaling(
            const AtlasSpec &spec, const std::string_view name)
        {
            if (halvesExactly(
                    static_cast<std::int32_t>(spec.spriteSize.width))
                && halvesExactly(
                    static_cast<std::int32_t>(spec.spriteSize.height))
                && halvesExactly(spec.pivot.x)
                && halvesExactly(spec.pivot.y))
            {
                return;
            }

            throw GfxError(
                "the " + std::string(name)
                + " atlas records a metric that does not scale "
                  "exactly at every zoom level");
        }
    }

    AtlasSpec specFrom(const antwika::atlas::AtlasMeta &meta) noexcept
    {
        return AtlasSpec{
            .spriteSize = meta.sprite,
            .pivot = meta.pivot,
            .isometric = meta.isometric,
            .columns = meta.columns,
            .rows = meta.rows};
    }

    AtlasSpecs specsFrom(
        const std::array<antwika::atlas::AtlasMeta, kAtlasKindCount>
            &metas,
        const antwika::atlas::AtlasMeta &walker) noexcept
    {
        AtlasSpecs specs;

        for (std::size_t index = 0; index < kAtlasKindCount; ++index)
        {
            specs.byKind[index] = specFrom(metas[index]);
        }

        specs.walker = specFrom(walker);

        return specs;
    }

    void requireAtlasSpecs(const AtlasSpecs &specs)
    {
        for (std::size_t index = 0; index < kAtlasKindCount; ++index)
        {
            const auto kind = static_cast<AtlasKind>(index);
            const auto &spec = specs.of(kind);
            const auto name = named(kind);

            requireSlots(spec, name);
            requireSprites(spec, kind);
            requireFootprint(spec, cellsOf(kind), name);
            requireScaling(spec, name);
        }

        requireSlots(specs.walker, kWalkerSheet);
        requireWalkRoom(specs.walker);
        requireFootprint(specs.walker, 1U, kWalkerSheet);
        requireScaling(specs.walker, kWalkerSheet);
    }

    void requireAtlasSize(
        const antwika::gfx::Bitmap &bitmap,
        const AtlasSpec &spec,
        const std::string_view name)
    {
        if (bitmap.size == spec.sheetSize())
        {
            return;
        }

        throw GfxError(
            "the texture atlas " + std::string(name) + " is "
            + describe(bitmap.size) + " but its metadata records a "
            + describe(spec.sheetSize()) + " one");
    }

    AtlasSheets loadAtlasSheets(const AtlasAssets &assets)
    {
        std::array<antwika::atlas::AtlasMeta, kAtlasKindCount> metas{};

        for (std::size_t index = 0; index < kAtlasKindCount; ++index)
        {
            metas[index] = atlasMetaAsset(assets.byKind[index]);
        }

        AtlasSheets sheets;
        sheets.specs = specsFrom(metas, atlasMetaAsset(assets.walker));

        requireAtlasSpecs(sheets.specs);

        for (std::size_t index = 0; index < kAtlasKindCount; ++index)
        {
            sheets.images[index] = antwika::app::readPngFile(
                antwika::app::assetPath(assets.byKind[index]),
                "antwika_game");

            requireAtlasSize(
                sheets.images[index],
                sheets.specs.byKind[index],
                assets.byKind[index]);
        }

        sheets.walker = antwika::app::readPngFile(
            antwika::app::assetPath(assets.walker), "antwika_game");

        requireAtlasSize(
            sheets.walker, sheets.specs.walker, assets.walker);

        return sheets;
    } // GCOVR_EXCL_LINE

    antwika::atlas::AtlasMeta atlasMetaAsset(const std::string_view name)
    {
        const auto path = antwika::atlas::metaPathFor(
            antwika::app::assetPath(name));

        auto meta = antwika::atlas::loadMetaFile(path);

        if (!meta.has_value())
        {
            throw GfxError(
                "antwika_game: the texture atlas " + std::string(name)
                + " ships without " + path);
        }

        return *meta;
    }

}
