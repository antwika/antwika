#pragma once

#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;

    /**
     * @brief Where a sheet is read from and written back to.
     *
     * The one seam between this application and a filesystem, and the
     * reason every other class here can be exercised with no file on
     * disk at all: a test hands the session a store that answers from
     * memory, and a session cannot tell the difference.
     *
     * It exists for a second reason too. antwika::gfx opens no files, so
     * *somebody* has to, and an application is where that somebody lives
     * -- see antwika::app::readPngFile, which says the same thing about
     * reading one.
     */
    class IAtlasStore
    {
    public:
        virtual ~IAtlasStore() = default;

        /**
         * @brief Read the sheet in.
         * @return The image, or nothing when this store was given
         * nowhere to read from -- which is what a session started on a
         * blank sheet has, and is an ordinary answer rather than a
         * failure.
         * @throws antwika::gfx::GfxError If there is somewhere to read
         * from and it cannot be read or decoded.
         */
        [[nodiscard]] virtual std::optional<Bitmap> load() = 0;

        /**
         * @brief Write the sheet out.
         * @param image The pixels to write.
         * @throws AtlasEditorError If this store was given nowhere to
         * write to.
         * @throws antwika::gfx::GfxError If the bytes cannot be written.
         */
        virtual void save(const Bitmap &image) = 0;

        /**
         * @brief Say where a save would go, for the status line.
         * @return The path, or an empty string when there is nowhere.
         */
        [[nodiscard]] virtual std::string savePath() const = 0;
    };

} // namespace antwika::atlas_editor
