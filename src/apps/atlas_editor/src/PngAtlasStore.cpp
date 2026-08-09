#include "antwika/atlas_editor/PngAtlasStore.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include <antwika/atlas/AtlasMetaFile.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/io/File.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::GfxError;

    PngAtlasStore::PngAtlasStore(
        std::optional<std::string> openPath,
        std::optional<std::string> writePath)
        : from(std::move(openPath)), to(std::move(writePath))
    {
    }

    std::optional<Bitmap> PngAtlasStore::load()
    {
        if (!from.has_value())
        {
            return std::nullopt;
        }

        return loadFrom(*from);
    }

    void PngAtlasStore::save(const Bitmap &image)
    {
        if (!to.has_value())
        {
            throw AtlasEditorError(
                "atlas_editor: no --out was given, so there is nowhere "
                "to save to");
        }

        saveTo(image, *to);
    }

    std::optional<Bitmap> PngAtlasStore::loadFrom(
        const std::string &path)
    {
        std::ifstream file = io::openToReadAs<GfxError>(
            path, "an image", io::Content::Bytes);

        return antwika::gfx::PngReader{}.read(file);
    }

    void PngAtlasStore::saveTo(
        const Bitmap &image, const std::string &path)
    {
        std::ofstream file = io::openToWriteAs<GfxError>(
            path, "an image", io::Content::Bytes);

        antwika::gfx::PngWriter{}.write(image, file);
    }

    std::optional<AtlasMeta> PngAtlasStore::loadMetaFrom(
        const std::string &path)
    {
        return antwika::atlas::loadMetaFile(
            antwika::atlas::metaPathFor(path));
    }

    void PngAtlasStore::saveMetaTo(
        const AtlasMeta &meta, const std::string &path)
    {
        antwika::atlas::storeMetaFile(
            meta, antwika::atlas::metaPathFor(path));
    }

    std::string PngAtlasStore::savePath() const
    {
        return to.value_or(std::string{});
    }

}
