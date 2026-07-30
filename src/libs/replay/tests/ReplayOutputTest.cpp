#include <gtest/gtest.h>

#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayWriter.hpp"
#include "ReplayOutput.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::detail::writeReplayOrThrow;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayWriter;

namespace
{
    // A device that takes no bytes, without being a device.
    // Refusing every character is what a full disk does to a write.
    class FullBuffer final : public std::streambuf
    {
    protected:
        int_type overflow(int_type) override
        {
            return traits_type::eof();
        }

        std::streamsize xsputn(const char *, std::streamsize) override
        {
            return 0;
        }
    };

    [[nodiscard]] std::vector<TickEvent> aSession()
    {
        return {
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = "game.score_increment",
                    .payload = "1",
                },
            },
        };
    }
} // namespace

// The failure this whole check exists for.
// A --record run writes once, at the end, and used to lose it silently.
TEST(ReplayOutputTest, ThrowsWhenTheStreamWillNotTakeTheBytes)
{
    FullBuffer buffer;
    std::ostream out(&buffer);

    try
    {
        writeReplayOrThrow(
            ReplayWriter(ReplayWriter::Layout::Compact),
            aSession(),
            out,
            "somewhere-full");
        FAIL() << "writing to a stream that takes nothing should throw";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("could not write"), std::string::npos)
            << message;
        EXPECT_NE(message.find("somewhere-full"), std::string::npos)
            << message;
    }
}

TEST(ReplayOutputTest, WritesTheDocumentToAStreamThatAcceptsIt)
{
    std::ostringstream out;

    writeReplayOrThrow(
        ReplayWriter(ReplayWriter::Layout::Compact),
        aSession(),
        out,
        "a-stringstream");

    EXPECT_NE(out.str().find("game.score_increment"), std::string::npos)
        << out.str();
}
