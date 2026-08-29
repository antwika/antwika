#pragma once

#include <optional>
#include <string>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/app/WindowInputSource.hpp"
#include "antwika/app/WindowPointerMapping.hpp"

namespace antwika::app
{

    using antwika::gfx::Size;
    using antwika::input::InputPipelineOptions;

    struct WindowedSessionSpec final
    {
        std::string name;

        std::string windowTitle;

        Size canvasSize;

        bool resizable = false;

        bool mapsPointerToCanvas = false;

        InputPipelineOptions input{};

        std::optional<std::string> replayPath{};

        std::string demoReplay{};
    };

}
