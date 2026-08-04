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

    /**
     * @brief What one windowed application differs from another by.
     *
     * Everything a main() used to state in the fifty lines before its
     * own wiring, and nothing else: the rest of those lines were the
     * same in every one of them, which is what WindowedSession now owns.
     */
    struct WindowedSessionDesc
    {
        /**
         * @brief What the run announces itself as in the log.
         *
         * "Antwika Life" produces "Antwika Life on backend: null,
         * input: null", which is the line every one of these
         * applications has always written.
         * It is separate from windowTitle because the two disagree:
         * apps/task_worker titles its window "Antwika Task Worker" and
         * logs "Antwika TaskWorker".
         */
        std::string name;

        /** @brief The title to ask the window system for. */
        std::string windowTitle;

        /**
         * @brief The size the window is asked for, and the surface
         * everything is laid out and hit-tested against.
         *
         * One field rather than two, because a layout resolved against
         * a size the window was not asked for is the bug
         * docs/resizable-windows.md exists to prevent.
         * canvas() hands it back, so what an application lays out
         * afterwards is laid against the same number.
         */
        Size canvas;

        /** @brief Whether the user may drag the window's edge. */
        bool resizable = false;

        /**
         * @brief Whether a device position is read as a canvas one.
         *
         * The mapping needs the window, which does not exist until this
         * description has been read, so it is asked for by a flag here
         * rather than handed over as an object.
         * On for a resizable window and for one that fills the screen,
         * since either makes the reported size differ from canvas.
         */
        bool mapsPointerToCanvas = false;

        /**
         * @brief What the application wants of its input.
         *
         * **readsDevice and pointerMapping are the session's**, and
         * whatever is set here for either is replaced: the first is a
         * property of the run rather than of the application, and the
         * second is built from the window this description opens.
         * Every other field is passed through untouched.
         */
        InputPipelineOptions input{};

        /** @brief What `--replay` named, if anything. */
        std::optional<std::string> replayPath{};

        /**
         * @brief The recording a run with no `--replay` is seeded with.
         *
         * Empty for an application that starts from nothing, which is
         * most of them.
         */
        std::string demoReplay{};
    };

    /**
     * @brief Everything a windowed application stands up before it
     * wires anything of its own.
     *
     * Ten main.cpp files opened with the same fifty lines: announce the
     * backends, open the window, seed a ReplaySource from `--replay` or
     * a demo, assemble the input pipeline and put a WindowInputSource
     * over it. The order of those is not a call site's business -- a
     * pipeline that reads a device during a replay is wrong in a way
     * that shows up as a divergent replay rather than as a failure --
     * so it is stated once here and each application says only what it
     * differs by.
     *
     * **It names nothing of antwika::console**, which depends on this
     * library. An application's own debug-console picture stays at its
     * own call site, laid against canvas() so it is still the size the
     * window was asked for and never one a window reports.
     *
     * **readsDevice is derived rather than passed**, from whether the
     * run was given a `--replay` path: a replay already holds the input
     * it recorded, so reading a device too would make every event
     * arrive twice. That was a line every main() wrote out by hand.
     *
     * It owns no logging and creates no backend, deliberately. The
     * makeSelected*Backend() factories are defined under backends/ and
     * linked by an application rather than by a library, so a library
     * calling one would drag a graphics framework into every test
     * binary that links antwika::app.
     *
     * What it does *not* cover is as deliberate: an application that
     * announces a third backend in the same line (apps/music_editor),
     * one that opens no window of its own (apps/poker), and one whose
     * preamble is interleaved with atlases and a locale (apps/game) all
     * keep their own, because bending either end to fit would cost more
     * than the repetition does.
     */
    class WindowedSession final
    {
    public:
        /**
         * @brief Announce the backends, open the window and assemble
         * the input.
         * @param logger Where the backend line is written, and what the
         * window is opened through; must outlive this object.
         * @param backend Opens the window and reports its events; must
         * outlive this object.
         * @param inputBackend Polled once a tick when a device is read;
         * must outlive this object.
         * @param desc What this application differs from another by.
         * @throws antwika::replay::ReplayFormatError If a named replay
         * or demo file cannot be read or parsed.
         */
        WindowedSession(
            ILogger &logger,
            IGfxBackend &backend,
            IInputBackend &inputBackend,
            const WindowedSessionDesc &desc);

        WindowedSession(const WindowedSession &) = delete;
        WindowedSession(WindowedSession &&) = delete;

        WindowedSession &operator=(const WindowedSession &) = delete;
        WindowedSession &operator=(WindowedSession &&) = delete;

        /**
         * @brief Get the window the session opened.
         * @return The window, for a renderer to draw into.
         */
        [[nodiscard]] IWindow &window() const noexcept;

        /**
         * @brief Get the size the window was asked for.
         * @return The canvas this session was described with, for
         * whatever an application lays out or hit-tests afterwards.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Get the codec the pipeline encodes edges with.
         * @return The one codec of the run, for a sink to decode with.
         */
        [[nodiscard]] const InputEventCodec &codec() const noexcept;

        /**
         * @brief Get the stream a tick's events are asked of.
         * @return The outermost source the session assembled, for an
         * application to wrap further or hand straight to its
         * bootstrap().
         */
        [[nodiscard]] ITickEventSource &source() noexcept;

        /**
         * @brief Get whether the graphics backend draws anything.
         * @return True under the backend that draws nothing, which is
         * also the one that reports no window close, so a run under it
         * has to be interrupted rather than closed.
         */
        [[nodiscard]] bool drawsNothing() const noexcept;

    private:
        // Declared in the order the main.cpp files built them in.
        // Each holds a reference into the one before it.
        // So none of this is copyable, movable or safe to reorder.
        std::unique_ptr<IWindow> opened;
        ReplaySource scripted;
        InputEventCodec encoding;

        // Built whether or not it is attached to the pipeline.
        // It is a window reference and a size, and costs nothing.
        // Attaching it is what desc.mapsPointerToCanvas decides.
        WindowPointerMapping mapping;

        antwika::input::InputPipeline pipeline;
        WindowInputSource windowed;
        Size surface;
        bool headless;
    };

} // namespace antwika::app
