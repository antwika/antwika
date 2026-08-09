#include "antwika/app/FramePreview.hpp"

#include <filesystem>
#include <string>

#include "antwika/app/AssetPath.hpp"
#include "antwika/app/PngFile.hpp"

namespace antwika::app
{

    std::string writtenPreview(
        const antwika::gfx::Bitmap &page, std::string_view name)
    {
        const auto directory =
            std::filesystem::path(executableDirectory()) / "preview";

        std::filesystem::create_directories(directory);

        const auto path =
            (directory / (std::string(name) + ".png")).string();

        writePngFile(page, path, "app.preview");

        return path;
    } // GCOVR_EXCL_LINE

}
