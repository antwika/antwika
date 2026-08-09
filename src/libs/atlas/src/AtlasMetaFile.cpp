#include "antwika/atlas/AtlasMetaFile.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/enums/FromName.hpp>

#include "antwika/atlas/AtlasError.hpp"
#include "antwika/atlas/AtlasMeta.hpp"

namespace antwika::atlas
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        constexpr std::string_view kMetaSuffix = ".json";

        constexpr antwika::enums::NameTable<AtlasKind>
            kKinds{{"isometric", "flat"}};

        constexpr std::int64_t kExtentLimit = 1 << 16;

        constexpr std::int64_t kPivotLimit = 1 << 16;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["kind"]["type"] = "string";

            for (const auto *count : {"columns", "rows"})
            {
                schema["properties"][count] =
                    wholeShape(0, kExtentLimit);
            }

            for (const auto *extent :
                 {"spriteWidth", "spriteHeight", "isometricWidth",
                  "isometricHeight"})
            {
                schema["properties"][extent] =
                    wholeShape(0, kExtentLimit);
            }

            for (const auto *at : {"pivotX", "pivotY"})
            {
                schema["properties"][at] =
                    wholeShape(-kPivotLimit, kPivotLimit);
            }
        }

        void encodeMembers(const AtlasMeta &meta, nlohmann::json &out)
        {
            out["kind"] = std::string(kKinds.name(meta.kind));
            out["columns"] = meta.columns;
            out["rows"] = meta.rows;
            out["spriteWidth"] = meta.sprite.width;
            out["spriteHeight"] = meta.sprite.height;
            out["pivotX"] = meta.pivot.x;
            out["pivotY"] = meta.pivot.y;
            out["isometricWidth"] = meta.isometric.width;
            out["isometricHeight"] = meta.isometric.height;
        }

        [[nodiscard]] AtlasKind kindFrom(const nlohmann::json &document)
        {
            if (!document.contains("kind"))
            {
                return AtlasKind::Isometric;
            }

            return antwika::enums::fromName<AtlasError>(
                kKinds,
                document.at("kind").get<std::string>(),
                "antwika::atlas: the atlas names a kind this build "
                "does not know: ");
        }

        AtlasMeta decodeMembers(const nlohmann::json &document)
        {
            AtlasMeta meta;
            meta.kind = kindFrom(document);
            meta.columns = memberOr(document, "columns", meta.columns);
            meta.rows = memberOr(document, "rows", meta.rows);
            meta.sprite.width =
                memberOr(document, "spriteWidth", meta.sprite.width);
            meta.sprite.height =
                memberOr(document, "spriteHeight", meta.sprite.height);
            meta.pivot.x = memberOr(document, "pivotX", meta.pivot.x);
            meta.pivot.y = memberOr(document, "pivotY", meta.pivot.y);
            meta.isometric.width = memberOr(
                document, "isometricWidth", meta.isometric.width);
            meta.isometric.height = memberOr(
                document, "isometricHeight", meta.isometric.height);

            return meta;
        }

        using MetaFormat =
            antwika::config::FileFormat<AtlasMeta, AtlasError>;

        [[nodiscard]] antwika::replay::MigrationChain migrations()
        {
            return antwika::replay::MigrationChain(
                {}, kAtlasMetaVersion); // GCOVR_EXCL_LINE
        }

        [[nodiscard]] const MetaFormat &format()
        {
            static const MetaFormat only{
                {.format =
                     {.magic = kAtlasMetaMagic,
                      .version = kAtlasMetaVersion},
                 .title = "antwika atlas editor atlas document",
                 .whatFailed = "antwika::atlas: the atlas JSON failed "
                               "schema validation: ",
                 .members = describeMembers,
                 .encode = encodeMembers,
                 .decode = decodeMembers,
                 .migrations = migrations}}; // GCOVR_EXCL_LINE

            return only;
        }
    }

    std::string metaPathFor(const std::string &image)
    {
        return image + std::string(kMetaSuffix);
    } // GCOVR_EXCL_LINE

    nlohmann::json metaToJson(const AtlasMeta &meta)
    {
        return format().toJson(meta);
    } // GCOVR_EXCL_LINE

    AtlasMeta metaFromJson(const nlohmann::json &document)
    {
        return format().fromJson(document);
    }

    std::optional<AtlasMeta> loadMetaFile(const std::string &path)
    {
        return format().loadFileIfPresent(path);
    }

    void storeMetaFile(const AtlasMeta &meta, const std::string &path)
    {
        format().storeFile(meta, path);
    }

}
