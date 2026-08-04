#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/poker/CashGame.hpp"
#include <antwika/console/ConsolePicture.hpp>

#include "antwika/poker/TableScene.hpp"

namespace antwika::poker
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::ITexture;

    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::holdem::Table;
    using antwika::time::ISleeper;

    /**
     * @brief Draws the table into a window once per engine tick.
     *
     * The output half of watching a game, and only that half: it reads
     * the table through a snapshot, draws, and never looks at an event
     * for anything but "was that a tick". Window input is somebody
     * else's job, which is what keeps rendering a write-only projection
     * -- see app::WindowCloseSource for the other half.
     *
     * Register it after whatever steps the table, or every frame shows
     * the tick before.
     */
    class TableRenderSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param window Drawn into; must outlive this object.
         * @param canvas The size the window was asked for.
         * The configured size rather than the size the window reports,
         * for the reason every other scene here lays out against one.
         * A layout is a function of its canvas, and the art is now a
         * function of that layout.
         * @param scene Turns a snapshot into drawing calls.
         * @param table Read for the state to draw.
         * @param game Read for who is sitting where.
         * @param sleeper Paces one frame per period.
         * @param framePeriod How long to hold each frame; zero draws as
         * fast as the ticks arrive.
         * @param tableName The name the table is announced under.
         * @param atlas The art the table is drawn from, or null to draw
         * only what antwika::ui can; must outlive this object.
         */
        TableRenderSink(
            IWindow &window,
            Size canvas,
            const TableScene &scene,
            const Table &table,
            const CashGame &game,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod,
            std::string tableName,
            OptionalAtlas atlas = std::nullopt,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                consolePicture = std::nullopt);

        TableRenderSink(const TableRenderSink &) = delete;
        TableRenderSink(TableRenderSink &&) = delete;

        TableRenderSink &operator=(const TableRenderSink &) = delete;
        TableRenderSink &operator=(TableRenderSink &&) = delete;

        /**
         * @brief Draw a frame if this event was an engine tick.
         * @param event The tick event to inspect.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Draw the table as it stands, and hold the frame.
         *
         * Does nothing once the window is closed.
         */
        void render() const;

    private:
        IWindow &window;
        Size canvas;
        const TableScene &scene;
        const Table &table;
        const CashGame &game;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
        std::string tableName;
        OptionalAtlas atlas;
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consolePicture;
    };

} // namespace antwika::poker
