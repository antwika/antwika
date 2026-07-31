#pragma once

#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>

#include "antwika/atlas_editor/IAtlasStore.hpp"

namespace antwika::atlas_editor
{

    /**
     * @brief A store that keeps the sheet in a PNG file.
     *
     * The two paths are separate so that `--image` can be opened without
     * being overwritten: an artist trying the editor on the game's own
     * atlas should have to name `--out` before anything of theirs can be
     * replaced.
     */
    class PngAtlasStore final : public IAtlasStore
    {
    public:
        /**
         * @brief Construct the store over the two paths it uses.
         * @param openPath The file to read, or nothing to start blank.
         * @param writePath The file to write, or nothing to refuse to
         * save at all.
         */
        PngAtlasStore(
            std::optional<std::string> openPath,
            std::optional<std::string> writePath);

        /**
         * @brief Read the sheet in.
         * @return The image, or nothing when no path to open was given.
         * @throws antwika::gfx::GfxError If the file cannot be opened,
         * or its bytes are not a PNG this can decode.
         */
        [[nodiscard]] std::optional<Bitmap> load() override;

        /**
         * @brief Write the sheet out.
         * @param image The pixels to write.
         * @throws AtlasEditorError If no path to write to was given.
         * @throws antwika::gfx::GfxError If the file cannot be opened
         * for writing, or the bytes cannot be written.
         */
        void save(const Bitmap &image) override;

        /**
         * @brief Say where a save would go.
         * @return The path, or an empty string when there is nowhere.
         */
        [[nodiscard]] std::string savePath() const override;

    private:
        std::optional<std::string> from;
        std::optional<std::string> to;
    };

} // namespace antwika::atlas_editor
