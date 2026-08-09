#pragma once

#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>

#include "antwika/atlas_editor/AtlasMeta.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;

    class IAtlasStore
    {
    public:
        virtual ~IAtlasStore() = default;

        [[nodiscard]] virtual std::optional<Bitmap> load() = 0;

        virtual void save(const Bitmap &image) = 0;

        /**
         * @brief Reads an image from a path the user named.
         *
         * @param path The file to read.
         * @return The image, or nothing where the file holds none.
         * @throws std::runtime_error If the file cannot be read.
         */
        [[nodiscard]] virtual std::optional<Bitmap> loadFrom(
            const std::string &path) = 0;

        /**
         * @brief Writes an image to a path the user named.
         *
         * @param image The image to write.
         * @param path The file to write it to.
         * @throws std::runtime_error If the file cannot be written.
         */
        virtual void saveTo(
            const Bitmap &image, const std::string &path) = 0;

        /**
         * @brief Reads the metadata recorded beside an image.
         *
         * @param path The image whose sidecar to read.
         * @return The metadata, or nothing where no sidecar exists.
         * @throws std::runtime_error If the sidecar cannot be read.
         */
        [[nodiscard]] virtual std::optional<AtlasMeta> loadMetaFrom(
            const std::string &path) = 0;

        /**
         * @brief Records metadata beside an image.
         *
         * @param meta The metadata to write.
         * @param path The image whose sidecar to write.
         * @throws std::runtime_error If the sidecar cannot be written.
         */
        virtual void saveMetaTo(
            const AtlasMeta &meta, const std::string &path) = 0;

        [[nodiscard]] virtual std::string savePath() const = 0;
    };

}
