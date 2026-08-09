#pragma once

#include <optional>
#include <set>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class GridSink final : public ITickEventSink
    {
    public:
        GridSink(
            World &world,
            PathIndex &paths,
            Camera &camera,
            GridExtent extent,
            SystemScheduler &scheduler,
            const InputFold &input,
            UiOverlay &overlay,
            const WorldMapState &cities,
            BuildingIndex &built,
            RoadDrag &drag,
            GameState &state,
            GameConfig config);

        GridSink(const GridSink &) = delete;
        GridSink(GridSink &&) = delete;

        GridSink &operator=(const GridSink &) = delete;
        GridSink &operator=(GridSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        void begin(Cell cell, std::optional<BuildTool> tool);
        void settle(Cell cell);
        void placeOne(Cell cell, BuildTool tool);
        void placePath(Cell cell);
        void placeBuilding(Cell cell, BuildingKind kind);
        void raze(Cell cell);
        void cancelToolOrPlaceWalker(Cell cell);
        void placeWalker(Cell cell);
        void beginRoadDrag(Cell cell);
        void endRoadDrag(Cell cell);
        void act(const antwika::input::InputEvent &event);

        World &world;
        PathIndex &paths;
        Camera &camera;
        GridExtent extent;
        SystemScheduler &scheduler;
        const InputFold &input;
        UiOverlay &overlay;
        const WorldMapState &cities;

        BuildingIndex &built;

        RoadDrag &drag;

        GameState &state;
        GameConfig config;
        std::optional<Cell> pressedAt{};
    };

}
