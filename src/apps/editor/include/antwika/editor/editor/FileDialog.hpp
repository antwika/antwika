#pragma once

#include <string>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/EditHistory.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DoubleClick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include "antwika/editor/editor/GameModule.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/plan/PlanFile.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/CharacterSheetView.hpp"
#include "antwika/editor/ui/ColorPicker.hpp"
#include "antwika/editor/ui/EditorBindings.hpp"
#include "antwika/editor/ui/IconsView.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/PlanView.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"
#include "antwika/rules/Gates.hpp"
#include "antwika/system/HealthSystem.hpp"
#include "antwika/system/OrientationSystem.hpp"
#include "antwika/system/PatrolSystem.hpp"

namespace antwika::editor
{

    struct FileDialog final
    {
        bool isSaveMode = false;

        std::string folder;

        std::string fileName;
    };

}
