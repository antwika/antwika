#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorTheme.hpp"
#include "antwika/atlas_editor/Ink.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    enum class FileItem : std::uint8_t
    {
        New = 0,

        Save,

        Load,

        Quit,
    };

    [[nodiscard]] constexpr FileItem enumBound(FileItem) noexcept
    {
        return FileItem::Quit;
    }

    inline constexpr std::size_t kFileItemCount =
        antwika::enums::kCount<FileItem>;

    enum class ViewItem : std::uint8_t
    {
        ZoomIn = 0,

        ZoomOut,

        Fit,

        Grid,

        Guides,

        Pivot,

        PixelGrid,

        PointerBorder,

        Preview,

        PreviewFocus,
    };

    [[nodiscard]] constexpr ViewItem enumBound(ViewItem) noexcept
    {
        return ViewItem::PreviewFocus;
    }

    inline constexpr std::size_t kViewItemCount =
        antwika::enums::kCount<ViewItem>;

    namespace widgets
    {
        inline constexpr WidgetId kFileMenu{1};

        inline constexpr WidgetId kViewMenu{2};

        inline constexpr WidgetId kStatusBar{3};

        inline constexpr WidgetId kFileField{4};

        inline constexpr WidgetId kFileConfirm{5};

        inline constexpr WidgetId kFileClose{6};

        inline constexpr WidgetId kFirstFileEntry{128};

        inline constexpr WidgetId kInkButton{7};

        inline constexpr WidgetId kToolRail{8};

        inline constexpr WidgetId kInkSwatch{9};

        inline constexpr WidgetId kFirstInkChannel{10};

        inline constexpr WidgetId kInkPanel{13};

        inline constexpr WidgetId kAtlasKind{14};

        inline constexpr WidgetId kAtlasCreate{15};

        inline constexpr WidgetId kFirstAtlasField{192};

        inline constexpr WidgetId kAtlasPresets{79};

        inline constexpr WidgetId kFirstAtlasPreset{80};

        inline constexpr WidgetId kFirstTool{16};

        inline constexpr WidgetId kFirstSwatch{32};

        inline constexpr WidgetId kFirstFileItem{48};

        inline constexpr WidgetId kFirstViewItem{64};

        inline constexpr WidgetId kSheetPane{24};

        inline constexpr WidgetId kPreviewPane{25};

        inline constexpr WidgetId kPreviewDivider{26};

        [[nodiscard]] constexpr WidgetId toolWidget(const Tool tool) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool)
                + static_cast<std::uint64_t>(tool));
        }

        [[nodiscard]] constexpr WidgetId swatchWidget(
            const std::size_t index) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstSwatch) + index);
        }

        [[nodiscard]] constexpr WidgetId fileItemWidget(
            const FileItem item) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstFileItem)
                + static_cast<std::uint64_t>(item));
        }

        [[nodiscard]] constexpr WidgetId fileEntryWidget(
            const std::size_t index) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstFileEntry) + index);
        }

        [[nodiscard]] constexpr WidgetId atlasPresetWidget(
            const std::size_t preset) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstAtlasPreset) + preset);
        }

        [[nodiscard]] constexpr WidgetId atlasFieldWidget(
            const std::size_t field) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstAtlasField) + field);
        }

        [[nodiscard]] constexpr WidgetId inkChannelWidget(
            const std::size_t channel) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstInkChannel) + channel);
        }

        [[nodiscard]] constexpr WidgetId viewItemWidget(
            const ViewItem item) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstViewItem)
                + static_cast<std::uint64_t>(item));
        }
    }

    static_assert(
        antwika::ui::assertDistinct(
            widgets::kFileMenu,
            widgets::kViewMenu,
            widgets::kStatusBar,
            widgets::kFileField,
            widgets::kFileConfirm,
            widgets::kFileClose,
            widgets::kFirstFileEntry,
            widgets::kToolRail,
            widgets::kInkButton,
            widgets::kInkSwatch,
            widgets::kInkPanel,
            widgets::kAtlasKind,
            widgets::kAtlasCreate,
            widgets::kAtlasPresets,
            widgets::atlasPresetWidget(0),
            widgets::atlasPresetWidget(kAtlasPresetCount - 1),
            widgets::atlasFieldWidget(0),
            widgets::atlasFieldWidget(kAtlasFieldCount - 1),
            widgets::inkChannelWidget(0),
            widgets::inkChannelWidget(kInkChannels - 1),
            widgets::fileItemWidget(FileItem::Quit),
            widgets::viewItemWidget(ViewItem::PreviewFocus),
            widgets::kSheetPane,
            widgets::kPreviewPane,
            widgets::kPreviewDivider,
            widgets::toolWidget(Tool::Paint),
            widgets::toolWidget(Tool::Erase),
            widgets::toolWidget(Tool::Fill),
            widgets::toolWidget(Tool::Pick),
            widgets::toolWidget(Tool::Select),
            widgets::toolWidget(Tool::Line),
            widgets::toolWidget(Tool::Ellipse),
            widgets::swatchWidget(0)),
        "every toolbar widget needs its own id");

    /**
     * @brief Writes out what a saved atlas records about itself.
     *
     * @param meta The atlas to describe.
     * @param translator The locale to write in.
     * @return One line per fact, in the order the save card shows
     *         them.
     */
    [[nodiscard]] std::array<std::string, kMetaLines> metaLines(
        const AtlasMeta &meta, const Translator &translator);

    [[nodiscard]] std::string statusLine(
        const EditorState &state, const Translator &translator);

    [[nodiscard]] Frame describeEditor(
        const EditorState &state,
        Pointer pointer,
        const Translator &translator,
        const antwika::ui::Keyboard &keyboard = {});

}
