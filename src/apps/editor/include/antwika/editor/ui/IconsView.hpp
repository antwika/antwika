#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>

namespace antwika::editor
{

    class IconsView final
    {
    public:
        void open(
            gfx::ViewportRenderer &viewportRenderer,
            gfx::Bitmap sheetBitmap);

        [[nodiscard]] gfx::ITexture *getTexture() const noexcept;

        [[nodiscard]] const gfx::Bitmap &getSheet() const noexcept;

        [[nodiscard]] gfx::ITexture *getChecker() const noexcept;

        [[nodiscard]] std::optional<std::size_t> getPicked()
            const noexcept;

        [[nodiscard]] bool isUnsaved() const noexcept;

        void keep() noexcept;

        void pick(std::optional<std::size_t> iconIndex) noexcept;

        void paint(
            gfx::ViewportRenderer &viewportRenderer,
            geometry::GridCell pixelCell,
            bool erases);

        void draw(gfx::ViewportRenderer &viewportRenderer) const;

        [[nodiscard]] std::size_t getCount() const;

    private:
        gfx::Bitmap iconSheet;
        std::optional<std::size_t> iconPicked = 0;
        bool iconsUnsaved = false;
        std::unique_ptr<gfx::ITexture> iconsTexture;
        std::unique_ptr<gfx::ITexture> iconCheckerTexture;
    };

}
