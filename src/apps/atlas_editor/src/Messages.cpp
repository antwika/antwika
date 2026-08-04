#include "antwika/atlas_editor/Messages.hpp"

#include <antwika/i18n/MessageTable.hpp>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    // Every array lists every id, in the same order.
    // A forgotten entry is a value-initialised hole, not a short array.
    // isComplete() below is what sees that hole.
    // A missing Swedish string is a red build, not a wrong label.
    constexpr i18n::MessageTable<MessageId> kMessageTable{
        .names{{
            {MessageId::ToolPaint, "ToolPaint"},
            {MessageId::ToolErase, "ToolErase"},
            {MessageId::ToolFill, "ToolFill"},
            {MessageId::ToolPick, "ToolPick"},
            {MessageId::ToolSelect, "ToolSelect"},
            {MessageId::ResetView, "ResetView"},
            {MessageId::Grid, "Grid"},
            {MessageId::Guides, "Guides"},
            {MessageId::Load, "Load"},
            {MessageId::Save, "Save"},
            {MessageId::PixelUnknown, "PixelUnknown"},
            {MessageId::PixelAt, "PixelAt"},
            {MessageId::Slot, "Slot"},
            {MessageId::SelectionSize, "SelectionSize"},
            {MessageId::Unsaved, "Unsaved"},
            {MessageId::Saved, "Saved"},
            {MessageId::SaveFailed, "SaveFailed"},
            {MessageId::NothingToLoad, "NothingToLoad"},
            {MessageId::Loaded, "Loaded"},
            {MessageId::LoadFailed, "LoadFailed"},
        }},
        .english{{
            {MessageId::ToolPaint, "PAINT"},
            {MessageId::ToolErase, "ERASE"},
            {MessageId::ToolFill, "FILL"},
            {MessageId::ToolPick, "PICK"},
            {MessageId::ToolSelect, "SELECT"},
            {MessageId::ResetView, "fit"},
            {MessageId::Grid, "grid"},
            {MessageId::Guides, "guides"},
            {MessageId::Load, "load"},
            {MessageId::Save, "save"},
            {MessageId::PixelUnknown, "px -,-"},
            {MessageId::PixelAt, "px {0},{1}"},
            {MessageId::Slot, "slot {0}"},
            {MessageId::SelectionSize, "sel {0}x{1}"},
            {MessageId::Unsaved, "UNSAVED"},
            {MessageId::Saved, "saved {0}"},
            {MessageId::SaveFailed, "save failed: {0}"},
            {MessageId::NothingToLoad, "nothing to load"},
            {MessageId::Loaded, "loaded"},
            {MessageId::LoadFailed, "load failed: {0}"},
        }},
        .swedish{{
            {MessageId::ToolPaint, "MÅLA"},
            {MessageId::ToolErase, "SUDDA"},
            {MessageId::ToolFill, "FYLL"},
            {MessageId::ToolPick, "PLOCKA"},
            {MessageId::ToolSelect, "MARKERA"},
            {MessageId::ResetView, "anpassa"},
            {MessageId::Grid, "rutnät"},
            {MessageId::Guides, "stödlinjer"},
            {MessageId::Load, "läs in"},
            {MessageId::Save, "spara"},
            {MessageId::PixelUnknown, "px -,-"},
            {MessageId::PixelAt, "px {0},{1}"},
            {MessageId::Slot, "ruta {0}"},
            {MessageId::SelectionSize, "mark {0}x{1}"},
            {MessageId::Unsaved, "OSPARAT"},
            {MessageId::Saved, "sparade {0}"},
            {MessageId::SaveFailed, "kunde inte spara: {0}"},
            {MessageId::NothingToLoad, "inget att läsa in"},
            {MessageId::Loaded, "inläst"},
            {MessageId::LoadFailed, "kunde inte läsa in: {0}"},
        }},
    };

    static_assert(
        i18n::isComplete(kMessageTable),
        "every MessageId needs a name and text in both locales");

} // namespace antwika::atlas_editor
