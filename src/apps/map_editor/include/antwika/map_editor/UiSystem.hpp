#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <optional>
#include <string>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
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

        void dragPaletteSquare(const ui::WidgetRects &rects);

        void drawPaletteOverlay(const ui::WidgetRects &rects);

        void refreshHint(ui::WidgetId hovered);

        void drawHint();

        void confirmDialog();

        [[nodiscard]] bool actMenus(
            const ui::Interactions &interactions);

        void menuAction(std::size_t menu, std::size_t entry);

        void saveCurrentSheet();

        void saveCurrentCharacter();

        void cycleView();

        void chooseEnemy(std::size_t index);

        void newCharacter();

        void deleteCharacterPressed();

        void setUiScale(std::uint32_t scale);

        void toggleFullscreen();

        void writeConfigNow();

        void press(ui::WidgetId activated);

        EditorStore &store;
        gfx::ViewportRenderer &view;
        gfx::IWindow &window;
        gfx::Size canvas;
        const console::ConsolePicture &console;
        log::ILogger &logger;
        std::unique_ptr<gfx::ITexture> svTexture{};
        std::optional<std::uint32_t> svTextureHue{};
        std::string hint{};
        HintKey hintKey{};
    };

}
