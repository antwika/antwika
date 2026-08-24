#pragma once

#include <cstdint>

#include <antwika/input/KeyModifiers.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/editor/editor/CameraRig.hpp"
#include "antwika/editor/editor/EditorDocument.hpp"
#include "antwika/editor/editor/PlaySession.hpp"
#include "antwika/editor/editor/state/Caption.hpp"
#include "antwika/editor/editor/state/FrameMeters.hpp"
#include "antwika/editor/view/IEditSteps.hpp"
#include "antwika/editor/view/INotices.hpp"
#include "antwika/editor/view/Workbench.hpp"
#include "antwika/editor/view/WorldRender.hpp"

namespace antwika::editor
{

    struct ViewContext final
    {
        EditorDocument &document;

        PlaySession &play;

        CameraRig &cameraRig;

        Caption &caption;

        FrameMeters &meters;

        time::SystemClock &clockSource;

        Workbench workbench;

        WorldRender render;

        IEditSteps &editSteps;

        INotices &notices;

        map::View shownView = map::View::World;

        input::KeyModifiers heldModifiers{};

        std::uint32_t tick = 0;
    };

}
