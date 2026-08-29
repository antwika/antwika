#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/SchemaVersionError.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplayVersions.hpp>
#include <antwika/replay/ReplayWriter.hpp>

using antwika::schema::getDocumentVersion;
using antwika::schema::SchemaVersionError;
using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::kTickEventSchemaVersion;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayWriter;

namespace
{
    std::vector<TickEvent> getOneEvent()
    {
        return {TickEvent{.tick = 3, .event = Event{.name = EventName{"a.b"}, .payload = "{}"}}};
    }

    std::string getReplayText(const std::string &versionMember)
    {
        return R"({"magic":"antwika-replay",)" + versionMember
               + R"("events":[{"tick":0,)"
                 R"("event":{"name":"a.b","payload":"{}"}}]})";
    }
}

TEST(ReplayVersioningTest, CurrentVersion_IsWhatThisBuildSupports)
{
    EXPECT_EQ(kReplayDocumentVersion, 2U);
    EXPECT_EQ(kTickEventSchemaVersion, 1U);
}

TEST(ReplayVersioningTest, DocumentVersion_ThrowsAReplayFormatError)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = getDocumentVersion(document); },
        ReplayFormatError);
}

TEST(ReplayVersioningTest, Write_RoundTripsAtTheCurrentVersion)
{
    std::ostringstream outputStream;
    ReplayWriter().write(getOneEvent(), outputStream);

    const std::string text = outputStream.str();
    const nlohmann::json header =
        nlohmann::json::parse(text.substr(0, text.find('\n')));
    EXPECT_EQ(getDocumentVersion(header), kReplayDocumentVersion);

    std::istringstream inputStream(text);
    EXPECT_EQ(ReplayReader().read(inputStream), getOneEvent());
}

TEST(ReplayVersioningTest, Read_TakesNoVersionAsVersionOne)
{
    std::istringstream inputStream(getReplayText(""));
    EXPECT_EQ(ReplayReader().read(inputStream).size(), 1U);
}

TEST(ReplayVersioningTest, Read_RefusesAndNamesANewerVersion)
{
    std::istringstream inputStream(getReplayText(R"("version":3,)"));
    try
    {
        std::ignore = ReplayReader().read(inputStream);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("version 3"), std::string::npos);
        EXPECT_NE(message.find("up to version 2"), std::string::npos);
    }
}

TEST(ReplayVersioningTest, Read_RefusesANewerVersionFromAHeader)
{
    std::istringstream inputStream(
        R"({"magic":"antwika-replay","version":9})"
        "\n");

    EXPECT_THROW(
        { std::ignore = ReplayReader().read(inputStream); },
        SchemaVersionError);
}

TEST(ReplayVersioningTest, Read_RefusesAnUnreachableVersion)
{
    std::istringstream inputStream(getReplayText(R"("version":0,)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(inputStream); },
        SchemaVersionError);
}

TEST(ReplayVersioningTest, Read_RefusesAMalformedVersionValue)
{
    std::istringstream inputStream(getReplayText(R"("version":"1",)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(inputStream); },
        SchemaVersionError);
}

TEST(ReplayVersioningTest, Read_RefusesANonObjectBySchema)
{
    std::istringstream inputStream("[1, 2, 3]");
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(inputStream); }, ReplayFormatError);
}
