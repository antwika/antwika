#pragma once

#include <string>
#include <string_view>

#include <antwika/gfx/Bitmap.hpp>

namespace antwika::app
{

    /**
     * @brief Read a PNG file off disk as a bitmap.
     *
     * Opening the file is the application's job, not the graphics
     * library's: antwika::gfx decodes bytes and never goes looking for
     * them, which is why saying a file is missing is an application's job
     * too. Every application that says it says it the same way, so it is
     * said here.
     *
     * @param path The file to read.
     * @param name The program's name, used to prefix a failure.
     * @return The decoded image.
     * @throws antwika::gfx::GfxError If the file cannot be opened, or its
     * bytes are not a PNG this can decode.
     */
    [[nodiscard]] antwika::gfx::Bitmap readPngFile(
        const std::string &path, std::string_view name);

} // namespace antwika::app
