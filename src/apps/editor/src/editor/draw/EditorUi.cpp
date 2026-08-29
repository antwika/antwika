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

#include "antwika/editor/ui/WidgetIds.hpp"

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

        if (!play.playing)
        {
            antwika::ui::paint(
                viewportRenderer.innerRenderer(), frame.drawList);

            if (auto *view = viewNow(); view != nullptr)
            {
                view->drawOverlay(viewContextNow());
            }

            drawToolHint(frame);
            drawCanvasHint();
        }

        viewportRenderer.present();

        recordFrameWork(startedAt);
    }

    ui::Frame Editor::layoutUi(
        const bool pressed, const bool buttonHeld)
    {
        antwika::ui::Context context{
            viewportRenderer.getWindowSize(),
            getGameTheme(),
            antwika::ui::Pointer{
                .positionPoint = pointer.pointerInWindow,
                .down = buttonHeld,
                .pressed = pressed,
                .extendsSelection = getHeldModifiers().shift,
                .scrolledSteps = pointer.wheelSteps},
            pressed ? antwika::ui::Keyboard{}
                    : antwika::ui::Keyboard{
                          .keys = keyBench.keysNow,
                          .typedText = keyBench.typedThisFrame},
            fileChooser.fileDialog.has_value() && fileChooser.fileDialog->isSaveMode
                ? antwika::editor::kPickerNameWidget
            : inkPanel.inkPicker.editingInk.has_value()
                ? kInkHexWidget
                : antwika::widget::kNoWidget};

        {
            context.setTheme(getMenuTheme());

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
                        .widgetId = getMenuWidget(menu),
                        .optionIdBaseWidget = getFirstItemWidget(menu),
                        .options = itemNamesOf(menu),
                        .markedOptions =
                            std::span<
                                const antwika::ui::
                                    OptionMark>(
                                markedText.data(), lines),
                        .placeholder = getMenuName(menu),
                        .open = dialogs.openMenu == menu});
            };

            for (const auto menu : antwika::editor::kEveryMenu)
            {
                dropMenu(menu);
            }

            context.spacer(antwika::ui::kGrowSizing);
            context.label(
                time::getFormatFrameRate(meters.frameRate.getPerSecond()),
                kTextColor);
        }

        {
            const auto tabs = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .backgroundColor = kPanelColor});

            for (const auto tab : kEveryView)
            {
                context.button(
                    std::string(getTabName(tab)),
                    antwika::ui::ButtonSpec{
                        .widgetId = getTabWidget(tab),
                        .state =
                            tab == viewChoice.activeView
                                 ? std::optional{
                                      antwika::ui::
                                          ButtonState::
                                              Pressed}
                                : std::nullopt});
            }

            context.spacer(antwika::ui::kGrowSizing);
        }

        context.setTheme(getGameTheme());

        if (layoutModals(context))
        {
        }
        else if (fileChooser.fileDialog.has_value())
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
                        .widthSizing = antwika::ui::getFixedSize(
                            kPickerWidth * kUiScale),
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(
                    context,
                    fileChooser.fileDialog->isSaveMode ? "Save the map as"
                                   : "Load a map");
                context.label(fileChooser.fileDialog->folder, kGridLineColor);

                context.button(
                    "..",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::kPickerParentFolderWidget,
                        .widthSizing = antwika::ui::kGrowSizing});

                for (std::size_t index = 0;
                     index < fileChooser.folderEntries.size();
                     ++index)
                {
                    context.button(
                        fileChooser.folderEntries.at(index) + "/",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::getMapRowWidget(index),
                            .widthSizing = antwika::ui::kGrowSizing});
                }

                for (std::size_t index = 0; index < fileChooser.mapEntries.size();
                     ++index)
                {
                    const auto entry = fileChooser.mapEntries.at(index);

                    context.button(
                        entry,
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::getMapRowWidget(
                                fileChooser.folderEntries.size() + index),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = entry == fileChooser.fileDialog->fileName
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }

                if (fileChooser.fileDialog->isSaveMode)
                {
                    context.textField(
                        antwika::ui::TextFieldSpec{
                            .widgetId = antwika::editor::
                                kPickerNameWidget,
                            .text = fileChooser.fileDialog->fileName,
                            .placeholder = "Name of the map",
                            .focused = true});
                }

                const auto buttonRow = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing});

                context.button(
                    fileChooser.fileDialog->isSaveMode ? "Save" : "Load",
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
        else if (
            auto *view = viewNow();
            view == nullptr || !view->layoutPanel(context, viewContextNow()))
        {
            const auto rowWidget = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .padding = viewChoice.activeView == View::Atlases
                             ? kUiScale * 2
                             : 0,
                    .gap = kUiScale * 2});

            if (isWorldShown() && !play.playing)
            {
                layoutEntityListPanel(context);
            }

            {
                const auto column = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing =
                            preferences.panelSizes.toolWidth == 0
                                ? antwika::ui::kFitSizing
                                : antwika::ui::getFixedSize(
                                      panelWidthOf(
                                          &PanelSizes::toolWidth,
                                          kMinPanelWidth)),
                        .heightSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = antwika::editor::kToolPanelWidget});

                if (isWorldShown())
                {
                    for (const auto group : kEveryToolGroup)
                    {
                        const auto members = getToolsIn(group);

                        panelTitle(
                            context,
                            std::string(getToolGroupTitle(group)));

                        for (std::size_t rowStart = 0;
                             rowStart < members.count;
                             rowStart += 2)
                        {
                            const auto pair = context.row(
                                antwika::ui::ContainerSpec{
                                    .widthSizing =
                                        antwika::ui::kFitSizing});

                            for (std::size_t rank = rowStart;
                                 rank < members.count
                                 && rank < rowStart + 2;
                                 ++rank)
                            {
                                const auto which =
                                    members.buttons.at(rank);
                                const auto active =
                                    isToolButtonActive(which);

                                context.iconButton(
                                    antwika::ui::Icon{
                                        .sheetTexture =
                                            iconsView.getTexture(),
                                        .sourceRect = iconOf(which),
                                        .scale = kUiScale},
                                    antwika::ui::ButtonSpec{
                                        .widgetId = getToolWidget(which),
                                        .state =
                                            active ? std::optional{
                                                     antwika::ui::
                                                         ButtonState::
                                                             Pressed}
                                               : std::nullopt});
                            }
                        }

                        if (group != ToolGroup::Voxel
                            || preferences.tool != Tool::Brush)
                        {
                            continue;
                        }

                        const auto shapes = context.row(
                            antwika::ui::ContainerSpec{
                                .widthSizing = antwika::ui::kFitSizing});

                        for (const auto which :
                             {Paint::Brush,
                              Paint::Line,
                              Paint::Rect})
                        {
                            context.iconButton(
                                antwika::ui::Icon{
                                    .sheetTexture = iconsView.getTexture(),
                                    .sourceRect = iconOf(which),
                                    .scale = kUiScale},
                                antwika::ui::ButtonSpec{
                                    .widgetId = getPaintWidget(which),
                                    .state =
                                        which == preferences.paint
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
                    panelTitle(context, "Tools");

                    for (const auto which : kEveryPaint)
                    {
                        const auto *shownView = viewNow();

                        if (shownView == nullptr
                            || !shownView->offersPaint(which))
                        {
                            continue;
                        }

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.getTexture(),
                                .sourceRect = iconOf(which),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = getPaintWidget(which),
                                .state =
                                    which == preferences.paint
                                           ? std::optional{
                                              antwika::ui::
                                                  ButtonState::
                                                      Pressed}
                                        : std::nullopt});
                    }

                    if (viewChoice.activeView == View::Character
                        && characterView.getMark().selection.has_value())
                    {
                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.getTexture(),
                                .sourceRect =
                                    antwika::editor::getMirrorIcon(),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = antwika::editor::
                                    kMirrorWidget});
                    }
                }

                const auto marking =
                    viewChoice.activeView == View::Atlases;

                if (marking)
                {
                    context.spacer(
                        antwika::ui::getFixedSize(
                            kPanelGap * kUiScale));

                    const auto kinds = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing});

                    for (const auto kind : voxel::kEveryKind)
                    {
                        const auto active =
                            stroke.selectedTile.has_value()
                            && getActiveRules(document.map, chosenLayer).kindOf(
                                   *stroke.selectedTile)
                                   == kind;

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.getTexture(),
                                .sourceRect = iconOf(kind),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = getKindWidget(kind),
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
                            stroke.selectedTile.has_value()
                            && getActiveRules(document.map, chosenLayer).facingOf(
                                   *stroke.selectedTile)
                                   == facing;

                        context.iconButton(
                            antwika::ui::Icon{
                                .sheetTexture = iconsView.getTexture(),
                                .sourceRect = iconOf(facing),
                                .scale = kUiScale},
                            antwika::ui::ButtonSpec{
                                .widgetId = getFacingWidget(facing),
                                .state =
                                    active ? std::optional{
                                             antwika::ui::
                                                 ButtonState::
                                                     Pressed}
                                       : std::nullopt});
                    }
                }

                if (viewChoice.activeView == View::Atlases)
                {
                    const auto parts = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing});

                    for (const auto part : kMarkedParts)
                    {
                        const auto active =
                            stroke.selectedTile.has_value()
                            && getActiveRules(document.map, chosenLayer).partOf(
                                   *stroke.selectedTile)
                                   == part;

                        context.button(
                            part == voxel::StairPart::Side
                                  ? "side"
                                  : "front",
                            antwika::ui::ButtonSpec{
                                .widgetId = getPartWidget(part),
                                .fillColor = active
                                           ? kSelectionAccentColor
                                           : kGridLineColor});
                    }
                }
            }

            context.edge(
                antwika::ui::EdgeSpec{
                    .widgetId = antwika::editor::kToolPanelEdgeWidget,
                    .panelWidget = antwika::editor::kToolPanelWidget,
                    .minimum = kMinPanelWidth,
                    .maximum = viewportRenderer.getWindowSize().width / 3,
                    .dragging =
                        pointer.heldEdgeWidget
                        == antwika::editor::kToolPanelEdgeWidget});

            if (isWorldPanelShown())
            {
                const auto world = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .heightSizing = antwika::ui::kGrowSizing,
                        .widgetId = antwika::editor::kWorldPanelWidget,
                        .clips = true});
            }

            if (const auto sheetNames = getSheetNames(viewChoice.activeView);
                sheetNames.has_value())
            {
                {
                    const auto middle = context.column(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::kGrowSizing});

                    panelTitle(context, std::string(sheetNames->sheetName));

                    const auto sheet = context.column(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kGrowSizing,
                            .heightSizing = antwika::ui::kGrowSizing,
                            .widgetId = antwika::editor::
                                kSheetPanelWidget,
                            .clips = true});
                }

                context.edge(
                    antwika::ui::EdgeSpec{
                        .widgetId = antwika::editor::kDrawColumnEdgeWidget,
                        .panelWidget = antwika::editor::kDrawColumnWidget,
                        .minimum = kMinPanelWidth,
                        .maximum =
                            viewportRenderer.getWindowSize().width / 3,
                        .dragging =
                            pointer.heldEdgeWidget
                            == antwika::editor::kDrawColumnEdgeWidget});

                const auto drawing = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::getFixedSize(
                            panelWidthOf(
                                &PanelSizes::inspectWidth,
                                getInspectColumnWidth(
                                    viewportRenderer.getWindowSize(),
                                    camera::kCanvasSize))),
                        .heightSizing = antwika::ui::kGrowSizing,
                        .widgetId = antwika::editor::kDrawColumnWidget});

                panelTitle(context, std::string(sheetNames->drawName));

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

            if (isWorldShown() && !play.playing
                && pointer.hoveredPosition.has_value())
            {
                const auto cubePosition = antwika::voxel::cubeIndexOf(
                    *pointer.hoveredPosition);

                context.label(
                    std::to_string(cubePosition.x) + " "
                        + std::to_string(cubePosition.y) + " "
                        + std::to_string(cubePosition.z)
                        + " - ",
                    kGridLineColor);
            }

            if (stroke.selectedTile.has_value()
                && (viewChoice.activeView == View::Atlases
                    || isWorldShown()))
            {
                const auto drawnTile = getEditedTile(document.map, chosenLayer, stroke, assignMode);
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
                time::getFormatFrameRate(meters.frameRate.getPerSecond()) + " - "
                    + time::getFormatFrameTime(
                          meters.workRate.getAverageFrameTime())
                    + " - w "
                    + time::getFormatFrameTime(meters.worldRate.getAverageFrameTime())
                    + " u " + time::getFormatFrameTime(
                        meters.uiRate.getAverageFrameTime())
                    + " s " + time::getFormatFrameTime(
                        meters.seamRate.getAverageFrameTime())
                    + " l " + time::getFormatFrameTime(
                        meters.lampRate.getAverageFrameTime())
                    + " c "
                    + time::getFormatFrameTime(meters.sightRate.getAverageFrameTime())
                    + " h "
                    + time::getFormatFrameTime(meters.hideRate.getAverageFrameTime()),
                kGridLineColor);
        }

        auto frame = context.build();
        const auto port = viewportRenderer.getViewport();
        const auto roomOf =
            [&frame, port](const antwika::widget::WidgetId id)
            -> std::optional<gfx::RectF>
        {
            const auto rect = frame.rects.getWidgetRect(id);

            if (!rect.has_value())
            {
                return std::nullopt;
            }

            return port.toCanvas(
                gfx::RectF(
                    {static_cast<float>(rect->originPoint.x),
                     static_cast<float>(rect->originPoint.y)},
                    {static_cast<float>(rect->size.width),
                     static_cast<float>(rect->size.height)}));
        };

        sheetView.sheetRect = roomOf(antwika::editor::kSheetPanelWidget);
        sheetView.canvasRect = roomOf(antwika::editor::kDrawPanelWidget);

        return frame;
    }

}
