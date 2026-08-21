#include <gtest/gtest.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <antwika/schema/fakes/FakeCountMigration.hpp>

#include "antwika/schema/JsonSchemas.hpp"
#include "antwika/schema/MigrationChain.hpp"
#include "antwika/schema/DocumentFormatError.hpp"
#include "antwika/schema/SchemaVersion.hpp"
#include "antwika/schema/SchemaVersionError.hpp"
#include "antwika/schema/VersionedDocument.hpp"

using antwika::schema::countSchema;
using antwika::schema::IMigration;
using antwika::schema::fakes::FakeCountMigration;
using antwika::schema::kSchemaVersionKey;
using antwika::schema::MigrationChain;
using antwika::schema::MigrationList;
using antwika::schema::kMaxDocumentDepth;
using antwika::schema::readVersionedDocument;
using antwika::schema::readVersionedRecord;
using antwika::schema::DocumentFormatError;
using antwika::schema::SchemaVersionError;

namespace
{
    class ToyFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    constexpr std::uint32_t kToyVersion = 2;

    MigrationChain toyMigrations()
    {
        MigrationList migrations;
        migrations.push_back(std::make_shared<FakeCountMigration>(
            1, kToyVersion));
        return MigrationChain(std::move(migrations), kToyVersion);
    }

    nlohmann::json toySchema()
    {
        nlohmann::json schema;
        schema["type"] = "object";
        schema["additionalProperties"] = false;
        schema["required"] = {"count"};
        schema["properties"]["count"] = countSchema();
        schema["properties"][std::string(kSchemaVersionKey)]["const"] =
            kToyVersion;
        return schema;
    }

    const nlohmann::json_schema::json_validator &toyValidator()
    {
        static const nlohmann::json_schema::json_validator validator(
            toySchema());
        return validator;
    }

    template <typename ErrorT>
    nlohmann::json read(const nlohmann::json &document)
    {
        return readVersionedDocument<ErrorT>(
            document,
            toyMigrations(),
            toyValidator(),
            "antwika::schema: a toy document is not one: ");
    }

    nlohmann::json current()
    {
        return nlohmann::json{
            {std::string(kSchemaVersionKey), kToyVersion}, {"count", 4}};
    }

    nlohmann::json pastTheBound()
    {
        nlohmann::json value = 7;

        for (std::size_t level = 0; level <= kMaxDocumentDepth;
             ++level)
        {
            nlohmann::json wrappedJson = nlohmann::json::array();
            wrappedJson.push_back(std::move(value));
            value = std::move(wrappedJson);
        }

        return value;
    }
}

TEST(VersionedDocumentTest, Current_ReturnsADocumentAtTheVersion)
{
    const auto document = read<ToyFormatError>(current());

    EXPECT_EQ(document.at("count"), 4);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)), kToyVersion);
}

TEST(VersionedDocumentTest, Current_RefusesADocumentNestedTooDeep)
{
    try
    {
        (void)read<ToyFormatError>(pastTheBound());
        FAIL() << "a document nested past the bound should have thrown";
    }
    catch (const ToyFormatError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("nests deeper"),
            std::string::npos)
            << error.what();
    }
}

TEST(VersionedDocumentTest, Current_RefusesARecordNestedTooDeep)
{
    EXPECT_THROW(
        (void)readVersionedRecord<ToyFormatError>(
            pastTheBound(),
            kToyVersion,
            toyMigrations(),
            toyValidator(),
            "antwika::schema: a toy record is not one: "),
        ToyFormatError);
}

TEST(VersionedDocumentTest, Current_MigratesBeforeItValidates)
{
    const auto document = read<ToyFormatError>(
        nlohmann::json{{std::string(kSchemaVersionKey), 1}});

    EXPECT_EQ(document.at("count"), 0);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)), kToyVersion);
}

TEST(VersionedDocumentTest, Current_ReportsAVersionFailureAsTheType)
{
    auto document = current();
    document[std::string(kSchemaVersionKey)] = kToyVersion + 1;

    EXPECT_THROW((void)read<ToyFormatError>(document), ToyFormatError);
}

TEST(VersionedDocumentTest, Current_KeepsTheChainsWording)
{
    auto document = current();
    document[std::string(kSchemaVersionKey)] = 99;

    try
    {
        (void)read<ToyFormatError>(document);
        FAIL() << "a newer document should have been refused";
    }
    catch (const ToyFormatError &error)
    {
        EXPECT_NE(std::string(error.what()).find("99"),
                  std::string::npos);
    }
}

TEST(VersionedDocumentTest, Current_ReportsASchemaFailureAsTheType)
{
    auto document = current();
    document["colour"] = "blue";

    EXPECT_THROW((void)read<ToyFormatError>(document), ToyFormatError);
}

TEST(VersionedDocumentTest, Current_NamesTheFormatOnASchemaFailure)
{
    auto document = current();
    document["count"] = -1;

    try
    {
        (void)read<ToyFormatError>(document);
        FAIL() << "a negative count should have been refused";
    }
    catch (const ToyFormatError &error)
    {
        EXPECT_NE(std::string(error.what()).find("a toy document"),
                  std::string::npos);
    }
}

TEST(VersionedDocumentTest, Current_LetsTheNarrowerErrorThrough)
{
    auto document = current();
    document[std::string(kSchemaVersionKey)] = kToyVersion + 1;

    EXPECT_THROW(
        (void)read<DocumentFormatError>(document), SchemaVersionError);
}

TEST(VersionedDocumentTest, Current_StillReportsSchemaAsTheBaseType)
{
    auto document = current();
    document.erase("count");

    EXPECT_THROW(
        (void)read<DocumentFormatError>(document), DocumentFormatError);
}
