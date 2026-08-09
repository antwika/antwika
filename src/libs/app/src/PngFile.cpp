#include "antwika/app/PngFile.hpp"

#include <fstream>
#include <ios>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/PngWriter.hpp>

namespace antwika::app
{

    antwika::gfx::Bitmap readPngFile(
        const std::string &path, std::string_view name)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw antwika::gfx::GfxError(
                std::string(name) + ": could not open an image: " + path);
        }

        return antwika::gfx::PngReader{}.read(file);
    }

    void writePngFile(
        const antwika::gfx::Bitmap &bitmap,
        const std::string &path,
        std::string_view name)
    {
        std::ofstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            throw antwika::gfx::GfxError(
                std::string(name) + ": could not write an image: " + path);
        }

        antwika::gfx::PngWriter{}.write(bitmap, file);
    }

}
