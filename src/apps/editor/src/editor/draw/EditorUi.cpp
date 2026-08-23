#include <antwika/ui/Painter.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/MapPicker.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::showStatus(
        const std::string &text,
        const bool warns,
        const std::uint32_t durationTicks)
    {
        statusMessageNotice.text = text;
        statusMessageNotice.expiresAtTick = tick + durationTicks;
        statusMessageNotice.warns = warns;
    }

    void Editor::finishView(
        const ui::Frame &frame,
        const std::chrono::time_point<std::chrono::system_clock>
            startedAt)
    {
        drawColorPicker();

        viewportRenderer.fillLetterbox(gfx::Color{});
        antwika::ui::paint(viewportRenderer.nativeRenderer(), frame.drawList);
        if (activeView == map::View::Plan)
        {
            plan.drawGhost(viewportRenderer, pointer.pointerInWindow);
        }

        drawToolHint(frame);
        drawCanvasHint();
        viewportRenderer.present();

        recordFrameWork(startedAt);
    }

    ui::Frame Editor::layoutUi(
        const bool pressed, const bool buttonHeld)
    {
        antwika::ui::Context context{
            viewportRenderer.windowSize(),
            gameTheme(),
            antwika::ui::Pointer{
                .positionPoint = pointer.pointerInWindow,
                .down = buttonHeld,
                .pressed = pressed,
                .extendsSelection = heldModifiers().shift},
            pressed ? antwika::ui::Keyboard{}
                    : antwika::ui::Keyboard{
                          .keys = keysNow,
                          .typedText = typedThisFrame},
            dialogs.fileDialog.has_value() && dialogs.fileDialog->isSaveMode
                ? antwika::editor::kPickerNameWidget
            : inkPicker.editingInk.has_value()
                ? decor::kInkHexWidget
                : antwika::widget::kNoWidget};

        {
            context.setTheme(menuTheme());

            const auto bar = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .backgroundColor = kPanelColor});

            const auto dropMenu =
                [&](const antwika::editor::Menu menu)
            {
                std::array<
                    antwika::ui::OptionMark,
                    kMaxMenuLines>
                    markedText{};
                std::size_t lines = 0;

                for (const auto item :
                     antwika::editor::itemsOf(menu))
                {
                    markedText.at(lines) =
                        !antwika::editor::isToggle(item)
                            ? antwika::ui::OptionMark::None
                        : isChecked(item)
                            ? antwika::ui::OptionMark::On
                            : antwika::ui::OptionMark::Off;
                    ++lines;
                }

                context.dropdown(
                    antwika::ui::DropdownSpec{
                        .widgetId = menuWidget(menu),
                        .optionIdBaseWidget = firstItemWidget(menu),
                        .options = itemNamesOf(menu),
                        .markedOptions =
                            std::span<
                                const antwika::ui::
                                    OptionMark>(
                                markedText.data(), lines),
                        .placeholder = menuName(menu),
                        .open = dialogs.openMenu == menu});
            };

            for (const auto menu : antwika::editor::kBarMenus)
            {
                dropMenu(menu);
            }

            if (dialogs.openMenu == antwika::editor::Menu::Settings)
            {
                dropMenu(antwika::editor::Menu::Settings);
            }

            context.spacer(antwika::ui::kGrowSizing);
            context.label(
                time::formatFrameRate(meters.frameRate.perSecond()),
                kTextColor);
        }

        {
            const auto tabs = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .backgroundColor = kPanelColor});

            for (const auto tab : map::kEveryView)
            {
                context.button(
                    std::string(tabName(tab)),
                    antwika::ui::ButtonSpec{
                        .widgetId = tabWidget(tab),
                        .state =
                            tab == activeView
                                 ? std::optional{
                                      antwika::ui::
                                          ButtonState::
                                              Pressed}
                                : std::nullopt});
            }

            context.spacer(antwika::ui::kGrowSizing);
        }

        context.setTheme(gameTheme());

        if (layoutModals(context))
        {
        }
        else if (dialogs.fileDialog.has_value())
        {
            const auto asking = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .crossAlignment = antwika::ui::Alignment::Center});

            context.spacer(antwika::ui::kGrowSizing);

            {
                const auto sheet = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::fixedSize(
                            kPickerWidth * kUiScale),
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(
                    context,
                    dialogs.fileDialog->isSaveMode ? "Save the map as"
                                   : "Load a map");
                context.label(dialogs.fileDialog->folder, kGridLineColor);

                context.button(
                    "..",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kPickerOverwriteWidget,
                        .widthSizing = antwika::ui::kGrowSizing});

                for (std::size_t index = 0;
                     index < dialogs.folderEntries.size();
                     ++index)
                {
                    context.button(
                        dialogs.folderEntries.at(index) + "/",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::mapRowWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing});
                }

                for (std::size_t index = 0; index < dialogs.mapEntries.size();
                     ++index)
                {
                    const auto entry = dialogs.mapEntries.at(index);

                    context.button(
                        entry,
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::mapRowWidget(
                                dialogs.folderEntries.size() + index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = entry == dialogs.fileDialog->fileName
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }

                if (dialogs.fileDialog->isSaveMode)
                {
                    context.textField(
                        antwika::ui::TextFieldSpec{
                            .widgetId = antwika::editor::
                                kPickerNameWidget,
                            .text = dialogs.fileDialog->fileName,
                            .placeholder = "Name of the map",
                            .focused = true});
                }

                const auto buttonRow = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                context.button(
                    dialogs.fileDialog->isSaveMode ? "Save" : "Load",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kPickerConfirmWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
                context.button(
                    "Cancel",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kPickerCancelWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
            }

            context.spacer(antwika::ui::kGrowSizing);
        }
        else if (activeView == map::View::Plan)
        {
            plan.layout(context, focusedField);
        }
        else
        {
            const auto rowWidget = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .padding = activeView == map::View::Atlases
                             ? kUiScale * 2
                             : 0,
                    .gap = kUiScale * 2});
            {
                const auto column = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kFitSizing,
                        .heightSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = antwika::editor::kToolPanelWidget});

                panelTitle(context, "Tools");

                if (activeView == map::View::World)
                {
                    for (std::size_t rowStart = 0;
                         rowStart < kEveryToolButton.size();
                         rowStart += 2)
                    {
                        const auto pair = context.row(
                            antwika::ui::ContainerSpec{
                                .widthSizing = antwika::ui::kFitSizing});

                        for (std::size_t rank = rowStart;
                             rank < kEveryToolButton.size()
                             && rank < rowStart + 2;
                             ++rank)
                        {
                            const auto which =
                                kEveryToolButton.at(rank);
                            const auto active = toolButtonActive(which);

                            context.iconButton(
                                antwika::ui::Icon{
                                    .sheetTexture = iconsView.texture(),
                                    .sourceRect = iconOf(which),
                                    .scale = kUiScale},
                                antwika::ui::ButtonSpec{
                                    .widgetId = toolWidget(which),
                                    .state =
                                        active ? std::optional{
                                                 antwika::ui::
                                                     ButtonState::
                                                         Pressed}
                                           : std::nullopt});
                        }
                    }

                    if (tool == map::Tool::Brush)
                    {
                        context.spacer(
                            antwika::ui::fixedSize(
                                kPanelGap * kUiScale));

                        for (const auto which :
                             {map::Paint::Brush,
                              map::Paint::Line,
                              map::Paint::Rect})
                        {
                            context.iconButton(
                                antwika::ui::Icon{
                                    .sheetTexture = iconsView.texture(),
                                    .sourceRect = iconOf(which),
                                    .scale = kUiScale},
                                antwika::ui::ButtonSpec{
                                    .widgetId = paintWidget(which),
                                    .state =
                                        which == paintMode
                                               ? std::optional{
                                                  antwika::ui::
                                                      ButtonState::
                                                          Pressed}
                                            : std::nullopt});
                        }
                    }
                }
                else
                {
                    for (const auto which : kEveryPaint)
                    {
                        if (which == map::Paint::Select
                            && activeView != map::View::Character)
                        {
                            continue;
                        }

                        if ((which == map::Paint::Rect
                             || which == map::Paint::Circle)
                            && activeView != map::View::Atlases)
                        {
                            continue;
                        }

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.texture(),
                                .sourceRect = iconOf(which),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = paintWidget(which),
                                .state =
                                    which == paintMode
                                           ? std::optional{
                                              antwika::ui::
                                                  ButtonState::
                                                      Pressed}
                                        : std::nullopt});
                    }

                    if (activeView == map::View::Character
                        && characterView.mark.selection.has_value())
                    {
                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.texture(),
                                .sourceRect =
                                    antwika::editor::mirrorIcon(),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = antwika::editor::
                                    kMirrorWidget});
                    }
                }

                const auto marking = activeView == map::View::World
                                     || activeView == map::View::Atlases;

                if (marking)
                {
                    context.spacer(
                        antwika::ui::fixedSize(
                            kPanelGap * kUiScale));

                    const auto kinds = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing});

                    for (const auto kind : voxel::kEveryKind)
                    {
                        const auto active =
                            activeView == map::View::World
                                        ? kind == brushKind
                                        : selectedTile.has_value()
                                      && activeRules().kindOf(
                                             *selectedTile)
                                             == kind;

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.texture(),
                                .sourceRect = iconOf(kind),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = kindWidget(kind),
                                .state =
                                    active ? std::optional{
                                             antwika::ui::
                                                 ButtonState::
                                                     Pressed}
                                       : std::nullopt});
                    }
                }

                if (marking)
                {
                    const auto facings = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing});

                    for (const auto facing : kMarkedFacings)
                    {
                        const auto active =
                            activeView == map::View::World
                                        ? facing == rampFacing
                                        : selectedTile.has_value()
                                      && activeRules().facingOf(
                                             *selectedTile)
                                             == facing;

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.texture(),
                                .sourceRect = iconOf(facing),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = facingWidget(facing),
                                .state =
                                    active ? std::optional{
                                             antwika::ui::
                                                 ButtonState::
                                                     Pressed}
                                       : std::nullopt});
                    }
                }

                if (activeView == map::View::Atlases)
                {
                    const auto parts = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing});

                    for (const auto part : kMarkedParts)
                    {
                        const auto active =
                            selectedTile.has_value()
                            && activeRules().partOf(
                                   *selectedTile)
                                   == part;

                        context.button(
                            part == voxel::StairPart::Side
                                  ? "side"
                                  : "front",
                            antwika::ui::ButtonSpec{
                                .widgetId = partWidget(part),
                                .fillColor = active
                                           ? kSelectionAccentColor
                                           : kGridLineColor});
                    }
                }
            }

            if (activeView == map::View::Atlases)
            {
                {
                    const auto middle = context.column(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::kGrowSizing});

                    panelTitle(context, "Tiles");

                    const auto sheet = context.column(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::kGrowSizing,
                            .widgetId = antwika::editor::
                                kSheetPanelWidget,
                            .clips = true});
                }

                const auto drawing = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::fixedSize(
                            inspectColumnWidth(
                                viewportRenderer.windowSize(),
                                camera::kCanvasSize)),
                        .heightSizing = antwika::ui::kGrowSizing});

                panelTitle(context, "Drawing");

                const auto canvasPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .heightSizing = antwika::ui::kGrowSizing,
                        .widgetId =
                            antwika::editor::kDrawPanelWidget,
                        .clips = true});
            }

            layoutSidebar(context);
        }

        {
            const auto strip = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .crossAlignment = antwika::ui::Alignment::Center,
                    .backgroundColor = kPanelColor,
                    .widgetId = antwika::editor::kStatusBarWidget});

            const auto fresh =
                tick < statusMessageNotice.expiresAtTick;
            const auto text =
                fresh ? statusMessageNotice.text : statusText();

            context.label(
                document.isDirty() ? "* " + text : text,
                fresh && statusMessageNotice.warns ? kForbiddenMarkerColor
                                                  : kTextColor);
            context.spacer(antwika::ui::kGrowSizing);

            if (activeView == map::View::World && !playing
                && pointer.hoveredPosition.has_value())
            {
                context.label(
                    std::to_string(pointer.hoveredPosition->x) + " "
                        + std::to_string(pointer.hoveredPosition->y) + " "
                        + std::to_string(pointer.hoveredPosition->z)
                        + " - ",
                    kGridLineColor);
            }

            if (selectedTile.has_value()
                && (activeView == map::View::Atlases
                    || activeView == map::View::World))
            {
                const auto drawnTile = editedTile();
                const auto index = static_cast<std::int32_t>(drawnTile.index);

                context.label(
                    std::string(
                        drawnTile.atlas == tilemap::Atlas::Wall ? "wall "
                                                                : "floor ")
                        + std::to_string(index % tilemap::kAtlasColumns) + " "
                        + std::to_string(index / tilemap::kAtlasColumns)
                        + " - ",
                    kGridLineColor);
            }

            context.label(
                time::formatFrameRate(meters.frameRate.perSecond()) + " - "
                    + time::formatFrameTime(
                          meters.workRate.averageFrameTime())
                    + " - w "
                    + time::formatFrameTime(meters.worldRate.averageFrameTime())
                    + " u " + time::formatFrameTime(
                        meters.uiRate.averageFrameTime())
                    + " s " + time::formatFrameTime(
                        meters.seamRate.averageFrameTime())
                    + " l " + time::formatFrameTime(
                        meters.lampRate.averageFrameTime())
                    + " c "
                    + time::formatFrameTime(meters.sightRate.averageFrameTime())
                    + " h "
                    + time::formatFrameTime(meters.hideRate.averageFrameTime()),
                kGridLineColor);
        }

        auto frame = context.build();
        const auto port = viewportRenderer.viewport();
        const auto roomOf =
            [&frame, port](const antwika::widget::WidgetId id)
            -> std::optional<gfx::RectF>
        {
            const auto rect = frame.rects.find(id);

            if (!rect.has_value())
            {
                return std::nullopt;
            }

            const auto share =
                static_cast<float>(port.denominator)
                / static_cast<float>(port.numerator);

            return gfx::RectF(
                {(static_cast<float>(rect->originPoint.x)
                  - static_cast<float>(port.offsetPoint.x))
                     * share,
                 (static_cast<float>(rect->originPoint.y)
                  - static_cast<float>(port.offsetPoint.y))
                     * share},
                {static_cast<float>(rect->size.width) * share,
                 static_cast<float>(rect->size.height)
                     * share});
        };

        sheetRect = roomOf(antwika::editor::kSheetPanelWidget);
        canvasRect = roomOf(antwika::editor::kDrawPanelWidget);

        return frame;
    }

}
