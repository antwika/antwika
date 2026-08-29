#pragma once

#include <memory>
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
#include "antwika/app/WindowedSessionSpec.hpp"

namespace antwika::app
{

    using antwika::event::ITickEventSource;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::input::IInputBackend;
    using antwika::input::InputEventCodec;
    using antwika::input::InputPipelineOptions;
    using antwika::log::ILogger;
    using antwika::replay::ReplaySource;

    class WindowedSession final
    {
    public:
        WindowedSession(
            ILogger &logger,
            IGfxBackend &backend,
            IInputBackend &inputBackend,
            const WindowedSessionSpec &spec);

        WindowedSession(const WindowedSession &) = delete;
        WindowedSession(WindowedSession &&) = delete;

        WindowedSession &operator=(const WindowedSession &) = delete;
        WindowedSession &operator=(WindowedSession &&) = delete;

        [[nodiscard]] IWindow &getWindow() const noexcept;

        [[nodiscard]] Size getCanvas() const noexcept;

        [[nodiscard]] const InputEventCodec &getCodec() const noexcept;

        [[nodiscard]] ITickEventSource &source() noexcept;

        [[nodiscard]] bool isHeadless() const noexcept;

    private:
        std::unique_ptr<IWindow> openedWindow;
        ReplaySource scriptedSource;
        InputEventCodec encodingCodec;

        WindowPointerMapping mapping;

        antwika::input::InputPipeline pipeline;
        WindowInputSource windowSource;
        Size canvasSize;
        bool headless;
    };

}
