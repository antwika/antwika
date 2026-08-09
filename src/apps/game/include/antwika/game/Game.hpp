#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include <antwika/console/ConsolePicture.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    class Game final
    {
    public:
        explicit Game(IEngine &engine, ILogger &logger);

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    struct GameWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        GridExtent extent;

        Camera &camera;

        PathIndex &paths;

        BuildingIndex &built;

        AppModeState &mode;

        PauseState &pause;

        std::optional<std::reference_wrapper<MapViewState>> view =
            std::nullopt;

        std::optional<std::reference_wrapper<DesirabilityField>>
            desirability = std::nullopt;

        std::optional<std::reference_wrapper<RoadDrag>> drag = std::nullopt;

        std::vector<std::reference_wrapper<ISystem>> observers = {};

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        std::optional<std::reference_wrapper<UiOverlay>> overlay =
            std::nullopt;

        std::optional<std::reference_wrapper<UiOverlay>> menuOverlay =
            std::nullopt;

        std::optional<std::reference_wrapper<WorldMapState>> world =
            std::nullopt;

        std::optional<std::reference_wrapper<UiOverlay>> saveOverlay =
            std::nullopt;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";

        std::vector<std::string> saves = {};

        std::string saveDirectory = {};

        std::optional<SaveGame> start = std::nullopt;

        std::optional<std::string> savePath = std::nullopt;

        std::optional<std::string> optionsPath = std::nullopt;

        std::uint64_t seed = 0;

        std::optional<std::reference_wrapper<LocaleState>>
            locale = std::nullopt;

        Size canvas = kUiCanvas;

        GameConfig config = {};
    };

    GameSummary bootstrap(const GameWiring &wiring);

    void printSummary(std::ostream &out, const GameSummary &summary);

}
