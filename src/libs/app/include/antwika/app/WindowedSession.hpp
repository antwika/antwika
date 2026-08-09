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
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/app/WindowInputSource.hpp"
#include "antwika/app/WindowPointerMapping.hpp"

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

    struct WindowedSessionDesc final
    {
        std::string name;

        std::string windowTitle;

        Size canvas;

        bool resizable = false;

        bool mapsPointerToCanvas = false;

        InputPipelineOptions input{};

        std::optional<std::string> replayPath{};

        std::string demoReplay{};
    };

    class WindowedSession final
    {
    public:
        WindowedSession(
            ILogger &logger,
            IGfxBackend &backend,
            IInputBackend &inputBackend,
            const WindowedSessionDesc &desc);

        WindowedSession(const WindowedSession &) = delete;
        WindowedSession(WindowedSession &&) = delete;

        WindowedSession &operator=(const WindowedSession &) = delete;
        WindowedSession &operator=(WindowedSession &&) = delete;

        [[nodiscard]] IWindow &window() const noexcept;

        [[nodiscard]] Size canvas() const noexcept;

        [[nodiscard]] const InputEventCodec &codec() const noexcept;

        [[nodiscard]] ITickEventSource &source() noexcept;

        [[nodiscard]] bool drawsNothing() const noexcept;

    private:
        std::unique_ptr<IWindow> opened;
        ReplaySource scripted;
        InputEventCodec encoding;

        WindowPointerMapping mapping;

        antwika::input::InputPipeline pipeline;
        WindowInputSource windowed;
        Size surface;
        bool headless;
    };

}
