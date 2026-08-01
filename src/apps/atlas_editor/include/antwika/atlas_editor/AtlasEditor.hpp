#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief How big a sheet a session starts on when it was given no
     * image to open.
     *
     * The game's own atlas, since that is the sheet this editor exists
     * to serve: eight columns by four rows of 128 by 64 tiles, as
     * wiki/apps/game-texture-atlas.md sets out.
     */
    inline constexpr Size kDefaultSheetSize{
        .width = 1024, .height = 256};

    /**
     * @brief What one session leaves behind, for a caller or a test.
     */
    struct EditorSummary
    {
        std::uint64_t ticks = 0;
        std::uint64_t edits = 0;
        std::uint32_t saves = 0;
        std::uint32_t loads = 0;
        Size image{};
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the sheet
     * needs the EditorState and the UiOverlay, and neither exists before
     * bootstrap() has opened an image.
     * Ownership passes back, so the sink lives exactly as long as the
     * session it belongs to.
     */
    using TickSinkFactory = std::function<std::unique_ptr<ITickEventSink>(
        const EditorState &, const UiOverlay &)>;

    /**
     * @brief Everything one session is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different session.
     */
    struct EditorConfig
    {
        /** @brief Receives the session's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickEventSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /** @brief Where the sheet is read from and written back to. */
        IAtlasStore &store;

        /**
         * @brief Words every label on the toolbar and status line.
         *
         * The bar is measured from what this says, and a press is
         * resolved against that layout, so the locale has to be the
         * same on the recording machine and the replaying one -- which
         * is why main() fixes it and reads one from nowhere else.
         */
        const Translator &translator;

        /**
         * @brief The size everything is laid out and hit-tested against.
         *
         * The size the window was *asked* for, never the size one
         * reports: which pixel a recorded click lands on is a function
         * of this, so it has to be the same number on the machine that
         * recorded a session and on the one replaying it.
         */
        Size canvas;

        /** @brief How big a sheet to open when the store has none. */
        Size blank = kDefaultSheetSize;

        /** @brief How to divide the sheet for the grid overlay. */
        TileGrid tiles = {};

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Unset runs uncapped, which is only ever right under a backend
         * that reports a window closing: the default `null` one reports
         * neither a close nor a key, so a run there would never end.
         * Tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /** @brief Sink receiving every dispatched event, tick-stamped. */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /** @brief Factory for one more tick sink, e.g. the renderer. */
        TickSinkFactory extraSink = {};
    };

    /**
     * @brief Open the sheet, wire the sinks up and run the loop.
     *
     * A live session and a replayed one are the same call: they differ
     * only in what inputSource was built from.
     *
     * @param config What the session is wired out of.
     * @return What the session ended on.
     * @throws antwika::gfx::GfxError If the store was given an image and
     * it cannot be read or decoded -- a `--image` that is not there is a
     * mistake worth stopping for, unlike a load somebody asked for
     * mid-session, which is reported in the status line instead.
     * @throws AtlasEditorError If the sheet to open holds no pixels.
     */
    EditorSummary bootstrap(const EditorConfig &config);

} // namespace antwika::atlas_editor
