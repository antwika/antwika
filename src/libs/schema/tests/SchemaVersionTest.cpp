#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <tuple>

#include <antwika/schema/DocumentFormatError.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/SchemaVersionError.hpp>

using antwika::schema::documentVersion;
using antwika::schema::DocumentFormatError;
using antwika::schema::kImplicitDocumentVersion;
using antwika::schema::kSchemaVersionKey;
using antwika::schema::SchemaVersionError;

TEST(SchemaVersionTest, CurrentVersion_IsWhatThisBuildSupports)
{
    EXPECT_EQ(kSchemaVersionKey, "version");
    EXPECT_EQ(kImplicitDocumentVersion, 1U);
}

TEST(SchemaVersionTest, DocumentVersion_IsNoneForANonObject)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::array()),
        kImplicitDocumentVersion);
}

TEST(SchemaVersionTest, DocumentVersion_ReadsAnAbsentMemberAsOne)
{
    EXPECT_EQ(
        documentVersion(nlohmann::json::object()),
        kImplicitDocumentVersion);
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
TEST(SchemaVersionTest, DocumentVersion_ThrowsADocumentFormatError)
{
    nlohmann::json document;
    document["version"] = "one";
    EXPECT_THROW(
        { std::ignore = documentVersion(document); },
        DocumentFormatError);
}
