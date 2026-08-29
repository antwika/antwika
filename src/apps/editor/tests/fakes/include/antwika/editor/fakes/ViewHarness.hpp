#pragma once

#include <cstddef>
#include <memory>

#include <antwika/assets/MapAssets.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/ui/GizmoSheet.hpp"

namespace antwika::editor::fakes
{

    class ViewHarness final
    {
    public:
        ViewHarness(
            log::ILogger &logger,
            IEditSteps &editStepsGiven,
            INotices &noticesGiven)
            : backend(logger),
              window(
                  backend.createWindow(
                      gfx::WindowSpec{
                          .title = "harness",
                          .size = camera::kCanvasSize})),
              viewportRenderer(
                  window->renderer(),
                  window->getSize(),
                  camera::kCanvasSize),
              play(logger, document.map, worldMeshes.getCells()),
              editSteps(&editStepsGiven),
              notices(&noticesGiven)
        {
            atlasSheets.take(
                assets::getLoadAtlasPairOrBlank(
                    document.getPath(), kAppName));
            characterView.open(
                viewportRenderer, character::getBlankCharacter());
            gizmos.sheetBitmap = getBlankGizmoSheet();
            gizmos.texture = viewportRenderer.createTexture(gizmos.sheetBitmap);
        }

        [[nodiscard]] ViewContext contextNow() noexcept
        {
            return ViewContext{
                .document = document,
                .play = play,
                .cameraRig = cameraRig,
                .caption = caption,
                .meters = meters,
                .clockSource = clockSource,
                .workbench =
                    Workbench{
                        .preferences = preferences,
                        .stroke = stroke,
                        .sheetView = sheetView,
                        .pointer = pointer,
                        .inkPicker = inkPicker,
                        .keyBench = keyBench,
                        .focusedField = focusedField,
                        .chosenLayer = chosenLayer,
                        .assignMode = assignMode,
                        .transition = transition,
                        .remesh = remesh,
                        .gizmos = gizmos,
                        .entityPick = entityPick},
                .render =
                    WorldRender{
                        .viewportRenderer = viewportRenderer,
                        .atlasSheets = atlasSheets,
                        .worldMeshes = worldMeshes,
                        .worldShader = worldShader,
                        .sprites = sprites,
                        .scenePass = scenePass,
                        .lightPasses = lightPasses,
                        .characterSkins = characterSkins},
                .editSteps = *editSteps,
                .notices = *notices,
                .shownView = View::World,
                .heldModifiers = {},
                .tick = 0};
        }

        gfx::NullBackend backend;
        std::unique_ptr<gfx::IWindow> window;
        gfx::ViewportRenderer viewportRenderer;
        render::WorldShader worldShader;
        render::Sprites sprites;
        render::WorldMeshes worldMeshes;
        render::AtlasSheets atlasSheets;
        render::ScenePass scenePass;
        render::LightPasses lightPasses;
        render::CharacterSkins characterSkins;
        EditorDocument document;
        PlaySession play;
        CameraRig cameraRig;
        Caption caption;
        FrameMeters meters;
        time::SystemClock clockSource;
        Preferences preferences;
        SheetStroke stroke;
        SheetView sheetView;
        PointerTrack pointer;
        InkPicker inkPicker;
        KeyBench keyBench;
        FocusedField focusedField = FocusedField::Nothing;
        std::size_t chosenLayer = map::kBaseLayer;
        AssignMode assignMode;
        TransitionPick transition;
        RemeshDebt remesh;
        GizmoSet gizmos;
        EntityPick entityPick;
        CharacterSheetView characterView;
        IEditSteps *editSteps;
        INotices *notices;
    };

}
