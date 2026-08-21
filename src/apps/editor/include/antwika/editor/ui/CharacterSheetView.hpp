#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/character/CharacterMarks.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/render/CharacterSkins.hpp>

#include "antwika/editor/ui/SheetMark.hpp"

namespace antwika::editor
{

    class CharacterSheetView final
    {
    public:
        void open(
            gfx::ViewportRenderer &viewportRenderer,
            gfx::Bitmap sheetBitmap);

        void takeSkins(
            gfx::ViewportRenderer &viewportRenderer,
            std::vector<gfx::Bitmap> skinBitmaps);

        [[nodiscard]] const std::vector<gfx::Bitmap> &skins()
            const noexcept;

        [[nodiscard]] std::vector<gfx::Bitmap> skinsAsDrawn() const;

        void keepEdits(gfx::ViewportRenderer &viewportRenderer);

        [[nodiscard]] std::size_t editing() const noexcept;

        void editFirst() noexcept;

        void switchTo(gfx::ViewportRenderer &viewportRenderer,
            std::size_t skinIndex);

        void repaint(
            gfx::ViewportRenderer &viewportRenderer,
            std::size_t skinIndex,
            gfx::Bitmap skinBitmap);

        [[nodiscard]] gfx::Bitmap &sheet() noexcept;

        [[nodiscard]] const gfx::Bitmap &sheet() const noexcept;

        void touch() noexcept;

        void refresh(gfx::ViewportRenderer &viewportRenderer);

        [[nodiscard]] gfx::ITexture *texture() const noexcept;

        [[nodiscard]] gfx::ITexture *skinTexture(
            std::size_t skinIndex) const noexcept;

        [[nodiscard]] gfx::ITexture *checker() const noexcept;

        void draw(gfx::ViewportRenderer &viewportRenderer) const;

        SheetMark mark;

    private:
        gfx::Bitmap editedSheet;
        std::size_t editingAt = 0;
        bool sheetDirty = true;
        render::CharacterSkins rosterSkins;
        std::unique_ptr<gfx::ITexture> sheetTexture;
        std::unique_ptr<gfx::ITexture> sheetCheckerTexture;
    };

}
