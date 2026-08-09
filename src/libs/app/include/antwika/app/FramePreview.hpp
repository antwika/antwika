#pragma once

#include <string>
#include <string_view>

#include <antwika/gfx/Bitmap.hpp>

namespace antwika::app
{

    /**
     * @brief Writes a drawn frame where a reader can find it.
     *
     * @param page The frame, as a BitmapWindow's page holds it.
     * @param name The file's name, without the .png suffix.
     * @return The path written, in a "preview" directory beside the
     *         running executable.
     * @throws antwika::gfx::GfxError If the file cannot be written.
     * @throws std::filesystem::filesystem_error If the directory
     *         cannot be made.
     */
    std::string writtenPreview(
        const antwika::gfx::Bitmap &page, std::string_view name);

}
