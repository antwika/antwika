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
#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class CharacterSheetView final : public IEditorView
    {
    public:
        [[nodiscard]] bool claims(
            map::View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        [[nodiscard]] bool takesPaintKeys() const noexcept override;

        [[nodiscard]] bool offersPaint(
            map::Paint paint) const noexcept override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        void commitFloatingPatch();

        void mirrorSelection(IEditSteps &editSteps);

        [[nodiscard]] bool consumePress(
            const ViewContext &viewContext,
            const input::PointerButtonPressed &downPressed) override;

        void trackPointer(const ViewContext &viewContext) override;

        [[nodiscard]] bool consumeKey(
            const ViewContext &viewContext,
            const input::KeyPressed &pressedKey) override;

        void open(
            gfx::ViewportRenderer &viewportRenderer,
            gfx::Bitmap sheetBitmap);

        void takeSkins(
            gfx::ViewportRenderer &viewportRenderer,
            render::CharacterSkins &rosterSkins,
            std::vector<gfx::Bitmap> skinBitmaps);

        [[nodiscard]] std::vector<gfx::Bitmap> getSkinsAsDrawn(
            const render::CharacterSkins &rosterSkins) const;



        void keepEdits(
            gfx::ViewportRenderer &viewportRenderer,
            render::CharacterSkins &rosterSkins);

        [[nodiscard]] std::size_t getEditing() const noexcept;

        void editFirst() noexcept;

        void switchTo(
            gfx::ViewportRenderer &viewportRenderer,
            render::CharacterSkins &rosterSkins,
            std::size_t skinIndex);

        void repaint(
            gfx::ViewportRenderer &viewportRenderer,
            render::CharacterSkins &rosterSkins,
            std::size_t skinIndex,
            gfx::Bitmap skinBitmap);

        [[nodiscard]] gfx::Bitmap &getSheet() noexcept;

        [[nodiscard]] const gfx::Bitmap &getSheet() const noexcept;

        void touch() noexcept;

        void refresh(gfx::ViewportRenderer &viewportRenderer);

        [[nodiscard]] gfx::ITexture *getTexture() const noexcept;



        [[nodiscard]] gfx::ITexture *getChecker() const noexcept;

        void drawSheet(gfx::ViewportRenderer &viewportRenderer) const;

        SheetMark mark;

    private:
        gfx::Bitmap editedSheet;
        std::size_t editingAt = 0;
        bool sheetDirty = true;
        std::unique_ptr<gfx::ITexture> sheetTexture;
        std::unique_ptr<gfx::ITexture> sheetCheckerTexture;
    };

}
