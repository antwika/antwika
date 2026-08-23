#include "antwika/image/PngFile.hpp"

#include <fstream>
#include <ios>

#include <antwika/io/SafeWrite.hpp>
#include <antwika/io/ScratchFile.hpp>

#include "antwika/gfx/GfxError.hpp"
#include "antwika/image/PngReader.hpp"
#include "antwika/image/PngWriter.hpp"

namespace antwika::image
{

    gfx::Bitmap getReadPngFile(const std::string &path, std::string_view name)
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
        io::ScratchFile writingFile{io::writingPathFor(path)};

        {
            std::ofstream file(writingFile.getPath(), std::ios::binary);

            if (!file.is_open())
            {
                throw gfx::GfxError(
                    std::string(name)
                    + ": could not write an image: " + writingFile.getPath());
            }

            PngWriter{}.write(bitmap, file);
        }

        io::putInPlaceKeepingBackup<gfx::GfxError>(
            writingFile.getPath(), path, name);
        writingFile.keep();
    }

}
