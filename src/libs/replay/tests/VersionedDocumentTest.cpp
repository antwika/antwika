#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "antwika/replay/JsonShapes.hpp"
#include "antwika/replay/MigrationChain.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/SchemaVersion.hpp"
#include "antwika/replay/SchemaVersionError.hpp"
#include "antwika/replay/VersionedDocument.hpp"

using antwika::replay::countShape;
using antwika::replay::IMigration;
using antwika::replay::kSchemaVersionKey;
using antwika::replay::MigrationChain;
using antwika::replay::MigrationList;
using antwika::replay::kMaxDocumentDepth;
using antwika::replay::readVersionedDocument;
using antwika::replay::readVersionedRecord;
using antwika::replay::ReplayFormatError;
using antwika::replay::SchemaVersionError;

namespace
{
    // A format of its own, with an error type of its own.
    // It stands in for the two applications that keep one.
    // SchemaVersionError is not a kind of this, which is the point.
    // A caller catching this must never be handed one.
    class ToyFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    constexpr std::uint32_t kToyVersion = 2;

    // Version 1 of this toy format wrote no "count" member at all.
    class AddCount final : public IMigration
    {
    public:
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override
        {
            return 1;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return kToyVersion;
        }

        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "toy-add-count";
        }

        void apply(nlohmann::json &document) const override
        {
            document["count"] = 0;
        }
    };

    MigrationChain toyMigrations()
    {
        MigrationList migrations;
        migrations.push_back(std::make_shared<AddCount>());
        return MigrationChain(std::move(migrations), kToyVersion);
    }

    // The one schema, describing the current version alone.
    // Which is what migrating before validating buys.
    nlohmann::json toySchema()
    {
        nlohmann::json schema;
        schema["type"] = "object";
        schema["additionalProperties"] = false;
        schema["required"] = {"count"};
        schema["properties"]["count"] = countShape();
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
            "antwika::replay: a toy document is not one: ");
    }

    nlohmann::json current()
    {
        return nlohmann::json{
            {std::string(kSchemaVersionKey), kToyVersion}, {"count", 4}};
    }

    // A number under one more level of arrays than the bound allows.
    nlohmann::json pastTheBound()
    {
        nlohmann::json value = 7;

        for (std::size_t level = 0; level <= kMaxDocumentDepth;
             ++level)
        {
            nlohmann::json wrapped = nlohmann::json::array();
            wrapped.push_back(std::move(value));
            value = std::move(wrapped);
        }

        return value;
    }
} // namespace

TEST(VersionedDocumentTest, ReturnsADocumentAtTheCurrentVersion)
{
    const auto document = read<ToyFormatError>(current());

    EXPECT_EQ(document.at("count"), 4);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)), kToyVersion);
}

// The guard runs before the migrate, the validate and every dump.
// A validator's failure message serialises the offending instance,
// and that serialisation recurses per nesting level.
TEST(VersionedDocumentTest, RefusesADocumentNestedPastTheBound)
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

// The record route opens with the very same guard.
TEST(VersionedDocumentTest, RefusesARecordNestedPastTheBound)
{
    EXPECT_THROW(
        (void)readVersionedRecord<ToyFormatError>(
            pastTheBound(),
            kToyVersion,
            toyMigrations(),
            toyValidator(),
            "antwika::replay: a toy record is not one: "),
        ToyFormatError);
}

// The order the whole mechanism rests on.
// A version 1 document satisfies no part of the one schema there is.
// It loads because it is migrated first, and only then checked.
TEST(VersionedDocumentTest, MigratesBeforeItValidates)
{
    const auto document = read<ToyFormatError>(
        nlohmann::json{{std::string(kSchemaVersionKey), 1}});

    EXPECT_EQ(document.at("count"), 0);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)), kToyVersion);
}

// A caller loading a toy document catches ToyFormatError alone.
// So a chain's own error may not be let out of this call.
TEST(VersionedDocumentTest, ReportsAVersionFailureAsTheCallersOwnType)
{
    auto document = current();
    document[std::string(kSchemaVersionKey)] = kToyVersion + 1;

    EXPECT_THROW((void)read<ToyFormatError>(document), ToyFormatError);
}

// And the chain's wording survives the translation.
// Which version was found is the only thing a person can act on.
TEST(VersionedDocumentTest, TheTranslatedMessageKeepsTheChainsWording)
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

TEST(VersionedDocumentTest, ReportsASchemaFailureAsTheCallersOwnType)
{
    auto document = current();
    document["colour"] = "blue";

    EXPECT_THROW((void)read<ToyFormatError>(document), ToyFormatError);
}

TEST(VersionedDocumentTest, TheSchemaFailureMessageNamesTheFormat)
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

// SchemaVersionError already narrows one error type: replay's own.
// Translating there would widen what a caller sees.
// So the narrower type comes through untouched.
// And a file from a newer release stays tellable from a corrupt one.
TEST(VersionedDocumentTest, LetsTheNarrowerVersionErrorThrough)
{
    auto document = current();
    document[std::string(kSchemaVersionKey)] = kToyVersion + 1;

    EXPECT_THROW(
        (void)read<ReplayFormatError>(document), SchemaVersionError);
}

TEST(VersionedDocumentTest, StillReportsASchemaFailureAsTheBaseType)
{
    auto document = current();
    document.erase("count");

    EXPECT_THROW(
        (void)read<ReplayFormatError>(document), ReplayFormatError);
}
