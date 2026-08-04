#pragma once

#include <optional>
#include <ostream>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>
#include <antwika/replay/ReplayWriter.hpp>

namespace antwika::replay
{

    /**
     * @brief The sink a `--record` run dispatches into: one line
     * appended and flushed per event, as it happens.
     *
     * **A recording is safe as soon as it is dispatched.** The whole
     * document a replay used to be could not be written until the run
     * had ended, and several applications have no end of their own --
     * so `Ctrl+C`, the ordinary way to stop one, saved nothing at all.
     * A record is a line here, written and flushed the moment the event
     * arrives, so a run killed part-way keeps everything up to the kill
     * and the file it leaves replays.
     *
     * Flushed, not synced: the bytes are handed to the operating system,
     * which is what makes them survive this process dying. Surviving the
     * machine dying would be an fsync per event, which costs milliseconds
     * each and buys a recording nothing -- there is no recording without
     * a process to have made it.
     *
     * `engine.tick` is filtered here and nowhere else. Engine::step()
     * regenerates it identically every run, live or replayed, so it was
     * never really input and must not be fed back in as replay input.
     * It is also the only name filtered: what lands in a recording is
     * decided by where the recorder sits, not by a list it skips.
     */
    class ReplayRecorder final : public event::ITickEventSink
    {
    public:
        /**
         * @brief Open a recording on a stream and write its header.
         * @param out Where the recording goes. Must outlive this object.
         * @param destination What to name in a failure, for a reader who
         * has to find the thing that would not take the bytes.
         * @param canvas The canvas this run laid its input out against,
         * stated in the header so a later run playing the recording back
         * against a different one can be told. Unset writes no canvas,
         * which is what a recording of an app with no pointer input has
         * to say.
         * @throws ReplayFormatError If the header cannot be written.
         * A path that will not take a header will not take a recording,
         * and finding that out at the end of a session is too late.
         */
        explicit ReplayRecorder(
            std::ostream &out,
            std::string destination,
            std::optional<geometry::Size> canvas = std::nullopt);

        ReplayRecorder(const ReplayRecorder &) = delete;
        ReplayRecorder(ReplayRecorder &&) = delete;
        ReplayRecorder &operator=(const ReplayRecorder &) = delete;
        ReplayRecorder &operator=(ReplayRecorder &&) = delete;
        ~ReplayRecorder() override = default;

        /**
         * @brief Append one event to the recording and flush it.
         * @param event The tick event to record.
         * @throws ReplayFormatError If the bytes cannot be written.
         * A full disk fails on the flush rather than on the open, and a
         * recorder that swallowed that would report a session it had not
         * kept.
         */
        void handle(const event::TickEvent &event) override;

    private:
        std::ostream &out;
        std::string destination;
        ReplayWriter writer;
    };

} // namespace antwika::replay
