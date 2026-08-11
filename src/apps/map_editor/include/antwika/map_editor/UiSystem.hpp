#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <memory>
#include <optional>
#include <string>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/ui/WidgetRects.hpp>

#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/Hints.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class UiSystem final : public ISystem
    {
    public:
        UiSystem(
            EditorStore &store,
            gfx::ViewportRenderer &view,
            gfx::IWindow &window,
            gfx::Size canvas,
            const console::ConsolePicture &console,
            std::string configPath,
            log::ILogger &logger);

        UiSystem(const UiSystem &) = delete;
        UiSystem(UiSystem &&) = delete;

        UiSystem &operator=(const UiSystem &) = delete;
        UiSystem &operator=(UiSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void act(
            const ui::Interactions &interactions,
            const ui::WidgetRects &rects);

        void actDialog(const ui::Interactions &interactions);

        void actPalette(
            const ui::Interactions &interactions,
            const ui::WidgetRects &rects);

        void actRules(const ui::Interactions &interactions);

        void actNewTileset(const ui::Interactions &interactions);

        void actBindings(const ui::Interactions &interactions);

        void actKeys(const ui::Interactions &interactions);

        void applyBindingsDialog();

        void openBindingsDialog();

        void chooseTileset(std::size_t index);

        void confirmTilesetDialog();

        [[nodiscard]] bool pressTilesets(ui::WidgetId activated);

        void dragPaletteSquare(const ui::WidgetRects &rects);

        void drawPaletteOverlay(const ui::WidgetRects &rects);

        void drawToolIcons(const ui::WidgetRects &rects);

        void drawBrushIcons(const ui::WidgetRects &rects);

        [[nodiscard]] const gfx::ITexture *terrainIconTexture(
            std::size_t at);

        void refreshHint(ui::WidgetId hovered);

        void drawHint();

        void confirmDialog();

        [[nodiscard]] bool actMenus(
            const ui::Interactions &interactions);

        void menuAction(std::size_t menu, std::size_t entry);

        void saveCurrentCharacter();

        void cycleView();

        void chooseEnemy(std::size_t index);

        void newCharacter();

        void deleteCharacterPressed();

        void setUiScale(std::uint32_t scale);

        void toggleFullscreen();

        void writeConfigNow();

        void press(ui::WidgetId activated);

        struct TerrainIcon final
        {
            std::string name{};
            std::uint64_t revision = 0;
            tilemap::Rgb ink{};
            tilemap::Rgb paper{};
            std::unique_ptr<gfx::ITexture> texture{};
        };

        static constexpr std::size_t kIconTerrains =
            enums::kCount<tilemap::TerrainClass>;

        EditorStore &store;
        gfx::ViewportRenderer &view;
        gfx::IWindow &window;
        gfx::Size canvas;
        const console::ConsolePicture &console;
        std::string configPath;
        log::ILogger &logger;
        std::unique_ptr<gfx::ITexture> svTexture{};
        std::optional<std::uint32_t> svTextureHue{};
        std::array<tileset::Tileset, kIconTerrains>
            iconPlaceholders{};
        std::array<TerrainIcon, kIconTerrains> terrainIcons{};
        std::string hint{};
        HintKey hintKey{};
    };

}
