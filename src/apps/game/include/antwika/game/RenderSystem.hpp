#pragma once

#include <functional>
#include <optional>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/time/Tick.hpp>

#include <antwika/console/ConsolePicture.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/AtlasTextures.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/RoadPlan.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    struct RenderSetup final
    {
        IWindow &window;

        const AppModeState &mode;

        Size canvas;

        const GridScene &scene;

        AtlasTextures atlases;

        const PathIndex &paths;

        const BuildingIndex &built;

        const Camera &camera;

        GridExtent extent;

        const PauseState &pause;

        const UiOverlay &overlay;

        std::optional<std::reference_wrapper<const MapViewState>> view =
            std::nullopt;

        std::optional<std::reference_wrapper<const DesirabilityField>>
            desirability = std::nullopt;

        std::optional<std::reference_wrapper<const RoadDrag>> drag =
            std::nullopt;

        const antwika::input::PointerHintChannel &hint;

        const MainMenuScene &menuScene;

        const UiOverlay &menuOverlay;

        const SaveLoadScene &saveScene;

        const UiOverlay &saveOverlay;

        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        const WorldMapScene &worldScene;

        const WorldMapState &cities;

        std::optional<std::reference_wrapper<FrameMeter>> fps =
            std::nullopt;
    };

    class RenderSystem final : public ISystem, public antwika::app::IFramePass
    {
    public:
        explicit RenderSystem(const RenderSetup &setup);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

        void draw(antwika::animation::Progress subTick) override;

    private:
        void drawScreen(
            antwika::gfx::IRenderer &renderer,
            antwika::animation::Progress subTick);

        void drawGrid(
            antwika::gfx::IRenderer &renderer,
            antwika::animation::Progress subTick);

        [[nodiscard]] RoadPlan planFor() const;

        RenderSetup setup;

        SceneSnapshot latest;
    };

}
