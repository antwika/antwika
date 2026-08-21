#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayRecorder.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::geometry::Size;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayRecorder;

namespace
{
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

    class TearingBuffer final : public std::streambuf
    {
    public:
        explicit TearingBuffer(const std::size_t tearsAfter)
            : tearsAfter(tearsAfter)
        {
        }

        [[nodiscard]] std::string text() const
        {
            return keptText;
        }

    protected:
        int_type overflow(int_type ch) override
        {
            if (keptText.size() >= tearsAfter)
            {
                return traits_type::eof();
            }
            keptText.push_back(traits_type::to_char_type(ch));
            return ch;
        }

    private:
        std::size_t tearsAfter;
        std::string keptText;
    };

    [[nodiscard]] TickEvent anEvent(
        const antwika::time::Tick tick, const std::string_view name)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = std::string(name), .payload = "1"}};
    }
}

TEST(ReplayRecorderTest, Ctor_ThrowsWhenTheHeaderWillNotWrite)
{
    FullBuffer buffer;
    std::ostream outputStream(&buffer);

    try
    {
        const ReplayRecorder recorder(outputStream, "somewhere-full");
        FAIL() << "opening onto a stream that takes nothing should throw";
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

TEST(ReplayRecorderTest, Handle_ThrowsWhenARecordWillNotWrite)
{
    TearingBuffer buffer(64);
    std::ostream outputStream(&buffer);
    ReplayRecorder recorder(outputStream, "somewhere-full");

    EXPECT_THROW(
        recorder.handle(anEvent(0, "game.score_increment")),
        ReplayFormatError);
}

TEST(ReplayRecorderTest, Ctor_WritesTheHeaderUpFront)
{
    std::ostringstream outputStream;
    const ReplayRecorder recorder(outputStream, "a-stringstream");

    EXPECT_NE(outputStream.str().find("antwika-replay"), std::string::npos)
        << outputStream.str();
}

TEST(ReplayRecorderTest, Handle_AppendsOneLinePerEvent)
{
    std::ostringstream outputStream;
    ReplayRecorder recorder(outputStream, "a-stringstream");

    recorder.handle(anEvent(0, "game.score_increment"));
    recorder.handle(anEvent(1, "game.score_increment"));

    const auto text = outputStream.str();
    EXPECT_EQ(std::count(text.begin(), text.end(), '\n'), 3);
}

TEST(ReplayRecorderTest, Handle_KeepsEveryEventBeforeAKill)
{
    std::ostringstream outputStream;
    {
        ReplayRecorder recorder(outputStream, "a-stringstream");
        recorder.handle(anEvent(0, "game.score_increment"));
        recorder.handle(anEvent(2, "game.score_increment"));

        std::istringstream mid(outputStream.str());
        EXPECT_EQ(ReplayReader().read(mid).size(), 2U);
    }
}

TEST(ReplayRecorderTest, Handle_FiltersOutTheRegeneratedTick)
{
    std::ostringstream outputStream;
    ReplayRecorder recorder(outputStream, "a-stringstream");

    recorder.handle(anEvent(0, "engine.tick"));
    recorder.handle(anEvent(0, "game.score_increment"));
    recorder.handle(anEvent(1, "engine.tick"));

    std::istringstream inputStream(outputStream.str());
    EXPECT_EQ(
        ReplayReader().read(inputStream),
        std::vector<TickEvent>{anEvent(0, "game.score_increment")});
}

TEST(ReplayRecorderTest, Ctor_StatesTheRecordedCanvas)
{
    std::ostringstream outputStream;
    const ReplayRecorder recorder(
        outputStream, "a-stringstream", Size{.width = 1024, .height = 640});

    EXPECT_NE(outputStream.str().find(R"("canvas")"), std::string::npos)
        << outputStream.str();
}

TEST(ReplayRecorderTest, Handle_LosesOnlyATornLastRecord)
{
    TearingBuffer buffer(120);
    std::ostream outputStream(&buffer);
    ReplayRecorder recorder(outputStream, "torn");

    recorder.handle(anEvent(0, "game.score_increment"));
    EXPECT_THROW(
        recorder.handle(anEvent(1, "game.score_increment")),
        ReplayFormatError);

    std::istringstream inputStream(buffer.text());
    EXPECT_EQ(
        ReplayReader().read(inputStream),
        std::vector<TickEvent>{anEvent(0, "game.score_increment")});
}
