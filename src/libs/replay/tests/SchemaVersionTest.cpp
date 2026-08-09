#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/replay/SchemaVersion.hpp>
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
}

TEST(SchemaVersionTest, CurrentVersion_IsWhatThisBuildSupports)
{
    EXPECT_EQ(kSchemaVersionKey, "version");
    EXPECT_EQ(kUnversionedDocumentVersion, 1U);
    EXPECT_EQ(kReplayDocumentVersion, 2U);
    EXPECT_EQ(kTickEventSchemaVersion, 1U);
}

TEST(SchemaVersionTest, DocumentVersion_IsNoneForANonObject)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::array()),
        kUnversionedDocumentVersion);
}

TEST(SchemaVersionTest, DocumentVersion_ReadsAnAbsentMemberAsOne)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::object()),
        kUnversionedDocumentVersion);
}

TEST(SchemaVersionTest, DocumentVersion_ReadsBackAStatedVersion)
{
    nlohmann::json document;
    document["version"] = 7;
    EXPECT_EQ(documentVersion(document), 7U);
}

TEST(SchemaVersionTest, DocumentVersion_AcceptsAnotherKey)
{
    nlohmann::json document;
    document["schemaVersion"] = 4;
    EXPECT_EQ(documentVersion(document, "schemaVersion"), 4U);
}

TEST(SchemaVersionTest, DocumentVersion_RefusesANonInteger)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, DocumentVersion_NamesAContainerVersion)
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

TEST(SchemaVersionTest, DocumentVersion_RefusesANegativeVersion)
{
    nlohmann::json document;
    document["version"] = -1;
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, DocumentVersion_RefusesAVersionPastUint32)
{
    nlohmann::json document;
    document["version"] = 4294967296ULL;
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, DocumentVersion_RefusesASignedOverflow)
{
    nlohmann::json document;
    document["version"] = std::int64_t{4294967296};
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        SchemaVersionError);
}

TEST(SchemaVersionTest, DocumentVersion_ReadsTheLargestVersionAUint32Holds)
{
    nlohmann::json document;
    document["version"] = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(
        documentVersion(document),
        std::numeric_limits<std::uint32_t>::max());
}

TEST(SchemaVersionTest, DocumentVersion_ReadsThatSameVersionWrittenSigned)
{
    nlohmann::json document;
    document["version"] = std::int64_t{4294967295};

    EXPECT_EQ(
        documentVersion(document),
        std::numeric_limits<std::uint32_t>::max());
}

TEST(SchemaVersionTest, DocumentVersion_SaysWhichNumbersItWouldHaveTaken)
{
    nlohmann::json document;
    document["version"] = "one";

    try
    {
        std::ignore = documentVersion(document);
        FAIL() << "a version that is not a number should have thrown";
    }
    catch (const SchemaVersionError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("from 0 to 4294967295"), std::string::npos)
            << message;
        EXPECT_NE(message.find(R"(found "one")"), std::string::npos)
            << message;
    }
}

TEST(SchemaVersionTest, DocumentVersion_ThrowsAReplayFormatError)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        ReplayFormatError);
}

TEST(SchemaVersionTest, Write_RoundTripsAtTheCurrentVersion)
{
    std::ostringstream out;
    ReplayWriter().write(oneEvent(), out);

    const std::string text = out.str();
    const nlohmann::json header =
        nlohmann::json::parse(text.substr(0, text.find('\n')));
    EXPECT_EQ(documentVersion(header), kReplayDocumentVersion);

    std::istringstream in(text);
    EXPECT_EQ(ReplayReader().read(in), oneEvent());
}

TEST(SchemaVersionTest, Read_TakesNoVersionAsVersionOne)
{
    std::istringstream in(replayText(""));
    EXPECT_EQ(ReplayReader().read(in).size(), 1U);
}

TEST(SchemaVersionTest, Read_RefusesAndNamesANewerVersion)
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

TEST(SchemaVersionTest, Read_RefusesANewerVersionFromAHeader)
{
    std::istringstream in(
        R"({"magic":"antwika-replay","version":9})"
        "\n");

    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, Read_RefusesAnUnreachableVersion)
{
    std::istringstream in(replayText(R"("version":0,)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, Read_RefusesAMalformedVersionValue)
{
    std::istringstream in(replayText(R"("version":"1",)"));
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, SchemaVersionError);
}

TEST(SchemaVersionTest, Read_RefusesANonObjectBySchema)
{
    std::istringstream in("[1, 2, 3]");
    EXPECT_THROW(
        { std::ignore = ReplayReader().read(in); }, ReplayFormatError);
}
