#include "antwika/app/WindowedSession.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/RunRecorded.hpp"

namespace antwika::app
{

    namespace
    {
        using antwika::gfx::WindowDesc;
        using antwika::log::Level;

        constexpr std::string_view kHeadlessBackendName = "null";

        std::unique_ptr<IWindow> announcedWindow(
            ILogger &logger,
            IGfxBackend &backend,
            IInputBackend &inputBackend,
            const WindowedSessionDesc &desc)
        {
            logger.log(
                Level::Info,
                desc.name + " on backend: " + std::string(backend.name())
                    + ", input: " + std::string(inputBackend.name()));

            WindowDesc window;
            window.title = desc.windowTitle;
            window.size = desc.canvas;
            window.resizable = desc.resizable;

            return backend.createWindow(window);
        }

        InputPipelineOptions attachedTo(
            const WindowedSessionDesc &desc,
            const WindowPointerMapping &mapping)
        {
            InputPipelineOptions options = desc.input;

            options.readsDevice = !desc.replayPath.has_value();

            if (desc.mapsPointerToCanvas)
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
        const WindowedSessionDesc &desc)
        : opened(announcedWindow(logger, backend, inputBackend, desc)),
          scripted(scriptedEvents(desc.replayPath, desc.demoReplay)),
          mapping(*opened, desc.canvas),
          pipeline(
              scripted,
              inputBackend,
              encoding,
              attachedTo(desc, mapping)),
          windowed(pipeline, backend, opened->id()),
          surface(desc.canvas),
          headless(backend.name() == kHeadlessBackendName)
    {
    }

    IWindow &WindowedSession::window() const noexcept
    {
        return *opened;
    }

    Size WindowedSession::canvas() const noexcept
    {
        return surface;
    }

    const InputEventCodec &WindowedSession::codec() const noexcept
    {
        return encoding;
    }

    ITickEventSource &WindowedSession::source() noexcept
    {
        return windowed;
    }

    bool WindowedSession::drawsNothing() const noexcept
    {
        return headless;
    }

}
