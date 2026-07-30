#pragma once

#include <ostream>
#include <string>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplayWriter.hpp>

/**
 * @file
 * @brief The half of saveReplayFile() that a real file is not needed to
 * exercise.
 *
 * Opening a destination and writing to it are two separate failures.
 * A path that cannot be opened is easy to ask for: name a directory
 * that is not there.
 * Bytes that cannot be written are not, and the only device that used to
 * be asked for it was /dev/full.
 * A skipped test covers nothing, so a runner without that device node
 * failed a 100% coverage gate over an environment difference.
 * Taking a stream rather than a path is what makes the failure something
 * a test can construct, on any machine, with no device at all.
 */
namespace antwika::replay::detail
{

    /**
     * @brief Write a replay document to a stream and confirm the bytes
     * got there.
     * @param writer The writer to encode with, carrying the layout and
     * the canvas the document should record.
     * @param events The events to write, in the order they occurred.
     * @param out The stream to write to.
     * @param destination What to name in the error, for a reader who
     * has to find the thing that would not take the bytes.
     * @throws ReplayFormatError If the stream is in a failed state once
     * the document has been written and flushed.
     */
    void writeReplayOrThrow(
        const ReplayWriter &writer,
        const std::vector<TickEvent> &events,
        std::ostream &out,
        const std::string &destination);

} // namespace antwika::replay::detail
