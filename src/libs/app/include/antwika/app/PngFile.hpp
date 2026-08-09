#pragma once

#include <string>
#include <string_view>

#include <antwika/gfx/Bitmap.hpp>

namespace antwika::app
{

    [[nodiscard]] antwika::gfx::Bitmap readPngFile(
        const std::string &path, std::string_view name);

    /**
     * @brief Writes an image out as a PNG.
     *
     * @param bitmap The image to write.
     * @param path Where to write it.
     * @param name The caller's name, for the message.
     * @throws antwika::gfx::GfxError If the file cannot be opened, or
     *         if the bitmap does not hold the pixels it claims.
     */
    void writePngFile(
        const antwika::gfx::Bitmap &bitmap,
        const std::string &path,
        std::string_view name);

}
