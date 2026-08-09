#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "antwika/atlas/AtlasMeta.hpp"

namespace antwika::atlas
{

    inline constexpr std::string_view kAtlasMetaMagic =
        "antwika-atlas-editor-atlas";

    inline constexpr std::uint32_t kAtlasMetaVersion = 1;

    /**
     * @brief Names the sidecar that carries an image's metadata.
     *
     * @param image The image path the sidecar sits beside.
     * @return That path with the sidecar suffix appended.
     */
    [[nodiscard]] std::string metaPathFor(const std::string &image);

    [[nodiscard]] nlohmann::json metaToJson(const AtlasMeta &meta);

    /**
     * @brief Reads metadata out of a sidecar document.
     *
     * @param document The parsed sidecar.
     * @return The metadata it describes.
     * @throws AtlasError If the document fails validation.
     */
    [[nodiscard]] AtlasMeta metaFromJson(const nlohmann::json &document);

    /**
     * @brief Reads the sidecar beside an image, if it has one.
     *
     * @param path The sidecar to read.
     * @return The metadata, or nothing where no such file exists.
     * @throws AtlasError If the file exists but cannot be read.
     */
    [[nodiscard]] std::optional<AtlasMeta> loadMetaFile(
        const std::string &path);

    /**
     * @brief Writes a sidecar beside an image.
     *
     * @param meta The metadata to write.
     * @param path The sidecar to write it to.
     * @throws AtlasError If the file cannot be written.
     */
    void storeMetaFile(const AtlasMeta &meta, const std::string &path);

}
