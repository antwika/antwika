#include <gtest/gtest.h>

#include <ostream>
#include <sstream>

#include "antwika/log/StreamAppender.hpp"

using antwika::log::StreamAppender;

namespace
{

    class SyncCountingBuffer final : public std::stringbuf
    {
    public:
        [[nodiscard]] int getSyncCount() const
        {
            return syncCount;
        }

    protected:
        int sync() override
        {
            ++syncCount;

            return std::stringbuf::sync();
        }

    private:
        int syncCount = 0;
    };

}

TEST(StreamAppenderTest, Append_WritesMessageToStream)
{
    std::ostringstream stream;
    StreamAppender appender(stream);
    appender.append("Message");
    EXPECT_EQ(stream.str(), "Message\n");
}

TEST(StreamAppenderTest, Append_FlushesEachAppendedLine)
{
    SyncCountingBuffer buffer;
    std::ostream stream(&buffer);
    StreamAppender appender(stream);

    appender.append("Message");

    EXPECT_EQ(buffer.getSyncCount(), 1);
}
