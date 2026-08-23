#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::render
{

    class AtlasSheets final
    {
    public:
        void open(
            gfx::IRenderer &viewportRenderer,
            std::array<gfx::Bitmap, 2> sheetBitmaps,
            const map::Map &drawnMap,
            std::uint32_t tick);

        void take(std::array<gfx::Bitmap, 2> sheetBitmaps);

        [[nodiscard]] gfx::Bitmap &sheet(std::size_t sheetIndex) noexcept;

        [[nodiscard]] gfx::Bitmap &sheet(tilemap::Atlas atlas) noexcept;

        [[nodiscard]] const std::array<gfx::Bitmap, 2> &getSheets()
            const noexcept;

        void touch() noexcept;

        [[nodiscard]] bool isTouched() const noexcept;

        void refresh(
            gfx::IRenderer &viewportRenderer,
            const map::Map &drawnMap,
            std::uint32_t tick,
            bool animating);

        [[nodiscard]] gfx::ITexture *getTexture(
            tilemap::Atlas atlas) const noexcept;

        [[nodiscard]] gfx::ITexture *getKeyed(
            tilemap::Atlas atlas) const noexcept;

        [[nodiscard]] gfx::ITexture *getChecker(
            tilemap::Atlas atlas) const noexcept;

    private:
        std::array<gfx::Bitmap, 2> bitmaps;
        bool dirty = false;
        std::array<std::unique_ptr<gfx::ITexture>, 2> paintedTextures;
        std::array<std::unique_ptr<gfx::ITexture>, 2> keyedOutTextures;
        std::array<std::unique_ptr<gfx::ITexture>, 2> checkerTextures;
    };

}
