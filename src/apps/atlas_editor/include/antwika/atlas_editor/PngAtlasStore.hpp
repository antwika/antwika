#pragma once

#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>

#include "antwika/atlas_editor/IAtlasStore.hpp"

namespace antwika::atlas_editor
{

    class PngAtlasStore final : public IAtlasStore
    {
    public:
        PngAtlasStore(
            std::optional<std::string> openPath,
            std::optional<std::string> writePath);

        [[nodiscard]] std::optional<Bitmap> load() override;

        void save(const Bitmap &image) override;

        [[nodiscard]] std::optional<Bitmap> loadFrom(
            const std::string &path) override;

        void saveTo(const Bitmap &image, const std::string &path)
            override;

        [[nodiscard]] std::optional<AtlasMeta> loadMetaFrom(
            const std::string &path) override;

        void saveMetaTo(const AtlasMeta &meta, const std::string &path)
            override;

        [[nodiscard]] std::string savePath() const override;

    private:
        std::optional<std::string> from;
        std::optional<std::string> to;
    };

}
