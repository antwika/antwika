#include "antwika/image/PngFile.hpp"

#include <fstream>
#include <ios>

#include <antwika/io/SafeWrite.hpp>

#include "antwika/gfx/GfxError.hpp"
#include "antwika/image/PngReader.hpp"
#include "antwika/image/PngWriter.hpp"

namespace antwika::image
{

    gfx::Bitmap readPngFile(const std::string &path, std::string_view name)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw gfx::GfxError(
                std::string(name) + ": could not open an image: " + path);
        }

        return PngReader{}.read(file);
    }

    void writePngFile(
        const gfx::Bitmap &bitmap,
        const std::string &path,
        std::string_view name)
    {
        const auto writingPath = io::writingPathFor(path);

        {
            std::ofstream file(writingPath, std::ios::binary);

            if (!file.is_open())
            {
                throw gfx::GfxError(
                    std::string(name)
                    + ": could not write an image: " + writingPath);
            }

            PngWriter{}.write(bitmap, file);
        }

        io::putInPlaceKeepingBackup<gfx::GfxError>(writingPath, path, name);
    }

}
