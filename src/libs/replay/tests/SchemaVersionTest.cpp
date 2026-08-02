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
    EXPECT_EQ(kReplayDocumentVersion, 2U);
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

// A container is named rather than printed.
// dump() recurses per nesting level, and what is stated here is
// unvalidated -- printing it is how a crafted file ate the stack.
TEST(SchemaVersionTest, AContainerVersionIsNamedNotPrinted)
{
    nlohmann::json document;
    document["version"] = nlohmann::json::array({1, 2});

    try
    {
        std::ignore = documentVersion(document);
        FAIL() << "a container version should have thrown";
    }
    catch (const SchemaVersionError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("a JSON array"),
            std::string::npos)
            << error.what();
    }
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

    // The header is the first line, and the only line that states one.
    const std::string text = out.str();
    const nlohmann::json header =
        nlohmann::json::parse(text.substr(0, text.find('\n')));
    EXPECT_EQ(documentVersion(header), kReplayDocumentVersion);

    std::istringstream in(text);
    EXPECT_EQ(ReplayReader().read(in), oneEvent());
}

TEST(SchemaVersionTest, ADocumentWithNoVersionReadsAsVersionOne)
{
    std::istringstream in(replayText(""));
    EXPECT_EQ(ReplayReader().read(in).size(), 1U);
}

TEST(SchemaVersionTest, ANewerVersionIsRefusedAndNamed)
{
    std::istringstream in(replayText(R"("version":3,)"));
    try
    {
        std::ignore = ReplayReader().read(in);
        FAIL() << "expected a SchemaVersionError";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("version 3"), std::string::npos);
        EXPECT_NE(message.find("up to version 2"), std::string::npos);
    }
}

// The message an older build gives somebody a JSON Lines replay.
// It parses the header and finds a version it does not know.
// And says so, rather than calling a file it cannot frame corrupt.
TEST(SchemaVersionTest, ANewerVersionIsRefusedFromAHeaderAlone)
{
    std::istringstream in(
        R"({"magic":"antwika-replay","version":9})"
        "\n");

    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, AVersionNoMigrationReachesCurrentFromIsRefused)
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
