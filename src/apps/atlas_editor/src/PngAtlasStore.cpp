#include "antwika/atlas_editor/PngAtlasStore.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <utility>

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

        std::ifstream file = io::openToReadAs<GfxError>(
            *from, "an image", io::Content::Bytes);

        return antwika::gfx::PngReader{}.read(file);
    }

    void PngAtlasStore::save(const Bitmap &image)
    {
        if (!to.has_value())
        {
            throw AtlasEditorError(
                "atlas_editor: no --out was given, so there is nowhere "
                "to save to");
        }

        // Scoped, never opened and closed by hand.
        std::ofstream file = io::openToWriteAs<GfxError>(
            *to, "an image", io::Content::Bytes);

        // The writer flushes and checks the stream itself.
        // A refusal by the filesystem arrives as a GfxError.
        // Without that a save could leave half a sheet in silence.
        antwika::gfx::PngWriter{}.write(image, file);
    }

    std::string PngAtlasStore::savePath() const
    {
        return to.value_or(std::string{});
    }

} // namespace antwika::atlas_editor
