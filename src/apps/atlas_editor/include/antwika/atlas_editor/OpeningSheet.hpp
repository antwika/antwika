#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/Canvas.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    namespace events
    {
        /** @brief Announces the sheet a session opened on. */
        inline constexpr std::string_view kOpeningSheet =
            "atlas.opening_sheet";
    } // namespace events

    /**
     * @brief Fingerprint a bitmap, deterministically.
     *
     * FNV-1a over the dimensions and every pixel byte: pure integer
     * arithmetic, so the same sheet answers the same on every run and
     * every toolchain.  Not a cryptographic hash and not meant to be;
     * what it guards against is a *different file*, not an adversary.
     *
     * @param image The pixels; must be complete.
     * @return The fingerprint.
     */
    [[nodiscard]] std::uint64_t fingerprintOf(
        const antwika::gfx::Bitmap &image) noexcept;

    /**
     * @brief Build the announcement for the sheet a session opened on.
     * @param canvas The sheet as it was opened, before any edit.
     * @return The event, carrying the fingerprint and the size.
     */
    [[nodiscard]] Event openingSheetEvent(const Canvas &canvas);

    /**
     * @brief Puts the opening sheet's announcement into the stream.
     *
     * The sheet a session opens on decides what every later click
     * means -- the Pick tool lifts a colour *off the sheet* -- yet it
     * used to reach the state through no event at all, so replaying a
     * session against a changed `--image` file diverged silently.
     * Announced ahead of the recorder, as sudoku's PuzzleSource
     * announces its grid, a recording carries the sheet it was drawn
     * on and EditorSink refuses a replay whose sheet is another.
     *
     * A replay run passes no announcement: the recorded one arrives
     * from the file instead, which is the whole point.
     */
    class OpeningSheetSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param announcement The event to put first, or nothing on a
         * replay run, whose recording already carries one.
         */
        OpeningSheetSource(
            ITickEventSource &inner, std::optional<Event> announcement);

        OpeningSheetSource(const OpeningSheetSource &) = delete;
        OpeningSheetSource(OpeningSheetSource &&) = delete;

        OpeningSheetSource &operator=(const OpeningSheetSource &) =
            delete;
        OpeningSheetSource &operator=(OpeningSheetSource &&) = delete;

        /**
         * @brief Get a tick's events, opening with the announcement
         * once.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, with the announcement
         * ahead of the first tick's.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Event> announcement;
    };

} // namespace antwika::atlas_editor
