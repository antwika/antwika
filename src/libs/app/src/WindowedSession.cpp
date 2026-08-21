#include "antwika/app/WindowedSession.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/RunRecorded.hpp"

namespace antwika::app
{

    namespace
    {
        using antwika::gfx::WindowSpec;
        using antwika::log::Level;

        constexpr std::string_view kHeadlessBackendName = "null";

        std::unique_ptr<IWindow> announcedWindow(
            ILogger &logger,
            IGfxBackend &backend,
            IInputBackend &inputBackend,
            const WindowedSessionSpec &spec)
        {
            logger.log(
                Level::Info,
                spec.name + " on backend: " + std::string(backend.name())
                    + ", input: " + std::string(inputBackend.name()));

            WindowSpec window;
            window.title = spec.windowTitle;
            window.size = spec.canvasSize;
            window.resizable = spec.resizable;

            return backend.createWindow(window);
        }

        InputPipelineOptions attachedTo(
            const WindowedSessionSpec &spec,
            const WindowPointerMapping &mapping)
        {
            InputPipelineOptions options = spec.input;

            options.readsDevice = !spec.replayPath.has_value();

            if (spec.mapsPointerToCanvas)
            {
                options.pointerMapping = mapping;
            }

            return options;
        }
    }

    WindowedSession::WindowedSession(
        ILogger &logger,
        IGfxBackend &backend,
        IInputBackend &inputBackend,
        const WindowedSessionSpec &spec)
        : openedWindow(announcedWindow(logger, backend, inputBackend, spec)),
          scriptedSource(loadReplayEvents(spec.replayPath, spec.demoReplay)),
          mapping(*openedWindow, spec.canvasSize),
          pipeline(
              scriptedSource,
              inputBackend,
              encodingCodec,
              attachedTo(spec, mapping)),
          windowSource(pipeline, backend, openedWindow->id()),
          canvasSize(spec.canvasSize),
          headless(backend.name() == kHeadlessBackendName)
    {
    }

    IWindow &WindowedSession::window() const noexcept
    {
        return *openedWindow;
    }

    Size WindowedSession::canvas() const noexcept
    {
        return canvasSize;
    }

    const InputEventCodec &WindowedSession::codec() const noexcept
    {
        return encodingCodec;
    }

    ITickEventSource &WindowedSession::source() noexcept
    {
        return windowSource;
    }

    bool WindowedSession::isHeadless() const noexcept
    {
        return headless;
    }

}
