#pragma once

#include <cstddef>

#include <antwika/voxel/VoxelMaterial.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/editor/state/AssignMode.hpp"
#include "antwika/editor/editor/state/EntityPick.hpp"
#include "antwika/editor/editor/state/FocusedField.hpp"
#include "antwika/editor/editor/state/GizmoSet.hpp"
#include "antwika/editor/editor/state/InkPicker.hpp"
#include "antwika/editor/editor/state/KeyBench.hpp"
#include "antwika/editor/editor/state/PointerTrack.hpp"
#include "antwika/editor/editor/state/RemeshDebt.hpp"
#include "antwika/editor/editor/state/SheetStroke.hpp"
#include "antwika/editor/editor/state/SheetView.hpp"
#include "antwika/editor/editor/state/TransitionPick.hpp"

namespace antwika::editor
{

    struct Workbench final
    {
        Preferences &preferences;

        SheetStroke &stroke;

        SheetView &sheetView;

        PointerTrack &pointer;

        InkPicker &inkPicker;

        const KeyBench &keyBench;

        FocusedField &focusedField;

        std::size_t &chosenLayer;

        AssignMode &assignMode;

        TransitionPick &transition;

        RemeshDebt &remesh;

        GizmoSet &gizmos;

        EntityPick &entityPick;
    };

}
