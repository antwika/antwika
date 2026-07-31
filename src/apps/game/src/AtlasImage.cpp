#include "antwika/game/AtlasImage.hpp"

#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] std::string describe(const antwika::gfx::Size &size)
        {
            return std::to_string(size.width) + "x"
                + std::to_string(size.height);
        }
    } // namespace

    void requireAtlasSize(const antwika::gfx::Bitmap &bitmap)
    {
        if (bitmap.size == kAtlasSize)
        {
            return;
        }

        // No app name in here.
        // runGuarded() puts one in front of whatever it catches.
        throw antwika::gfx::GfxError(
            "the texture atlas is " + describe(bitmap.size)
            + " but TileAtlas.hpp addresses a " + describe(kAtlasSize)
            + " one");
    }

} // namespace antwika::game
