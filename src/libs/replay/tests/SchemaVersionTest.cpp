#include <antwika/replay/SchemaVersion.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplayWriter.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::documentVersion;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::kSchemaVersionKey;
using antwika::replay::kTickEventSchemaVersion;
using antwika::replay::kUnversionedDocumentVersion;
using antwika::replay::ReplayFormatError;
using antwika::replay::ReplayReader;
using antwika::replay::ReplayWriter;
using antwika::replay::SchemaVersionError;

namespace
{
    std::vector<TickEvent> oneEvent()
    {
        return {TickEvent{.tick = 3, .event = Event{"a.b", "{}"}}};
    }

    std::string replayText(const std::string &versionMember)
    {
        return R"({"magic":"antwika-replay",)" + versionMember
               + R"("events":[{"tick":0,)"
                 R"("event":{"name":"a.b","payload":"{}"}}]})";
    }
} // namespace

TEST(SchemaVersionTest, ConstantsAreTheOnesThisBuildSupports)
{
    EXPECT_EQ(kSchemaVersionKey, "version");
    EXPECT_EQ(kUnversionedDocumentVersion, 1U);
    EXPECT_EQ(kReplayDocumentVersion, 1U);
    EXPECT_EQ(kTickEventSchemaVersion, 1U);
}

TEST(SchemaVersionTest, NonObjectDocumentHasNoStatedVersion)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::array()),
        kUnversionedDocumentVersion);
}

TEST(SchemaVersionTest, AbsentMemberReadsAsVersionOne)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::object()),
        kUnversionedDocumentVersion);
}

TEST(SchemaVersionTest, StatedVersionIsReadBack)
{
    nlohmann::json document;
    document["version"] = 7;
    EXPECT_EQ(documentVersion(document), 7U);
}

TEST(SchemaVersionTest, AnotherKeyCanBeNamed)
{
    nlohmann::json document;
    document["schemaVersion"] = 4;
    EXPECT_EQ(documentVersion(document, "schemaVersion"), 4U);
}

TEST(SchemaVersionTest, NonIntegerVersionIsRefused)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, NegativeVersionIsRefused)
{
    nlohmann::json document;
    document["version"] = -1;
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, VersionBeyondThirtyTwoBitsIsRefused)
{
    nlohmann::json document;
    document["version"] = 4294967296ULL;
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, SignedVersionBeyondThirtyTwoBitsIsRefused)
{
    nlohmann::json document;
    document["version"] = std::int64_t{4294967296};
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, ASchemaVersionErrorIsAReplayFormatError)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        ReplayFormatError);
}

TEST(SchemaVersionTest, WriterAndReaderRoundTripAtTheCurrentVersion)
{
    std::ostringstream out;
    ReplayWriter().write(oneEvent(), out);

    const nlohmann::json written = nlohmann::json::parse(out.str());
    EXPECT_EQ(documentVersion(written), kReplayDocumentVersion);

    std::istringstream in(out.str());
    EXPECT_EQ(ReplayReader().read(in), oneEvent());
}

TEST(SchemaVersionTest, ADocumentWithNoVersionReadsAsVersionOne)
{
    std::istringstream in(replayText(""));
    EXPECT_EQ(ReplayReader().read(in).size(), 1U);
}

TEST(SchemaVersionTest, ANewerVersionIsRefusedAndNamed)
{
    std::istringstream in(replayText(R"("version":2,)"));
    try
    {
        std::ignore = ReplayReader().read(in);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("version 2"), std::string::npos);
        EXPECT_NE(message.find("reads version 1"), std::string::npos);
    }
}

TEST(SchemaVersionTest, AnOlderVersionThisBuildDroppedIsRefused)
{
    std::istringstream in(replayText(R"("version":0,)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, AMalformedVersionValueIsRefused)
{
    std::istringstream in(replayText(R"("version":"1",)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, ADocumentThatIsNotAnObjectIsRefusedByTheSchema)
{
    std::istringstream in("[1, 2, 3]");
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, ReplayFormatError);
}
