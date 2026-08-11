#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/autotile/TileDraw.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/TilesetPreview.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class MapRenderSystem final : public ISystem
    {
    public:
        MapRenderSystem(
            EditorStore &store, gfx::ViewportRenderer &view);

        MapRenderSystem(const MapRenderSystem &) = delete;
        MapRenderSystem(MapRenderSystem &&) = delete;

        MapRenderSystem &operator=(const MapRenderSystem &) = delete;
        MapRenderSystem &operator=(MapRenderSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        struct BakedTileset final
        {
            std::string name{};
            std::uint64_t revision = 0;
            std::unique_ptr<gfx::ITexture> texture{};
            tileset::AtlasIndex index{};
        };

        struct ResolvedTileset final
        {
            const tileset::Tileset *set = nullptr;
            const gfx::ITexture *texture = nullptr;
            const tileset::AtlasIndex *index = nullptr;
            std::string name{};
            std::uint64_t revision = 0;
        };

        struct CharacterTexture final
        {
            std::string name{};
            std::uint64_t revision = 0;
            std::unique_ptr<gfx::ITexture> texture{};
        };

        struct PlanKey final
        {
            std::uint64_t mapRevision = 0;
            std::array<std::string, kTerrainCount> names{};
            std::array<std::uint64_t, kTerrainCount> revisions{};
            geometry::GridCell player{};
            std::int32_t playerHeight = 0;
            std::uint64_t clockBucket = 0;

            [[nodiscard]] bool operator==(
                const PlanKey &other) const = default;
        };

        struct PreviewKey final
        {
            std::string name{};
            std::uint64_t revision = 0;
            std::size_t layer = 0;
            std::size_t sprite = 0;
            std::uint32_t seed = 0;

            [[nodiscard]] bool operator==(
                const PreviewKey &other) const = default;
        };

        void drawMap(antwika::time::Tick tick);

        void drawBackdrop();

        void drawGhost();

        void drawHoverOutline();

        [[nodiscard]] std::optional<gfx::Point>
        mapViewPointer() const;

        void updatePicker(
            const std::array<ResolvedTileset, kTerrainCount>
                &resolved,
            antwika::time::Tick tick);

        void applyPick(
            const std::array<ResolvedTileset, kTerrainCount>
                &resolved,
            gfx::Point point,
            antwika::time::Tick tick);

        [[nodiscard]] std::string pickHoverText(
            const std::array<ResolvedTileset, kTerrainCount>
                &resolved,
            gfx::Point point) const;

        void drawPickerPreview(antwika::time::Tick tick);

        void drawFreeMarks();

        void drawTiles(antwika::time::Tick tick);

        [[nodiscard]] const TilesetPreview *refreshPreview(
            const TilesetDoc *doc);

        void refreshTilesets();

        void rebakeStatic();

        void bakeInto(
            BakedTileset &slot,
            const tileset::Tileset &set,
            std::uint64_t revision);

        [[nodiscard]] BakedTileset *findBaked(
            const std::string &name);

        [[nodiscard]] std::array<ResolvedTileset, kTerrainCount>
        resolveBindings();

        void refreshCharacters();

        void drawCharacters(antwika::time::Tick tick);

        EditorStore &store;
        gfx::ViewportRenderer &view;
        tilemap::Rgb bakedInk{};
        tilemap::Rgb bakedPaper{};
        std::array<tileset::Tileset, kTerrainCount> placeholders{};
        std::array<BakedTileset, kTerrainCount> placeholderBaked{};
        std::vector<BakedTileset> baked{};
        gfx::Bitmap systemArt{};
        std::unique_ptr<gfx::ITexture> systemTexture{};
        std::vector<CharacterTexture> characterTextures{};
        autotile::DrawPlan cachedPlan{};
        std::optional<PlanKey> planKey{};
        TilesetPreview cachedPreview{};
        std::optional<PreviewKey> previewKey{};
    };

}
