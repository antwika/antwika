#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    /**
     * @brief The numbers this application reads off config.json
     * beside its assets at startup.
     *
     * Every field defaults to the value it externalizes, so a
     * default-constructed one is the shipped application and a
     * missing file changes nothing. The format's mechanics live in
     * antwika::config; what may not move here is anything a recorded
     * click's meaning depends on, for the reasons apps/game's page
     * gives.
     */
    struct AtlasEditorConfig
    {
        /** @brief Milliseconds one frame takes on the wall clock. */
        std::int32_t framePeriodMs = 40;
    };

} // namespace antwika::atlas_editor
