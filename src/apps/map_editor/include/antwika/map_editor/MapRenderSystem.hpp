#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

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
        using Sheets = std::array<
            std::unique_ptr<gfx::ITexture>,
            enums::kCount<tilemap::TerrainClass>>;

        struct CharacterTexture final
        {
            std::string name{};
            std::uint64_t revision = 0;
            std::unique_ptr<gfx::ITexture> texture{};
        };

        void drawMap(antwika::time::Tick tick);

        void drawBackdrop();

        void drawGhost();

        void drawHoverOutline();

        void drawFreeMarks();

        void refreshSheets();

        void refreshCharacters();

        void drawCharacters(antwika::time::Tick tick);

        EditorStore &store;
        gfx::ViewportRenderer &view;
        Sheets sheets;
        std::array<
            std::uint64_t,
            enums::kCount<tilemap::TerrainClass>>
            revisions{};
        std::vector<CharacterTexture> characterTextures{};
    };

}
