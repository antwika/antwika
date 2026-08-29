#pragma once

#include <nlohmann/json.hpp>

#include <antwika/loadout/ComponentValue.hpp>

namespace antwika::loadout
{

    [[nodiscard]] nlohmann::json getComponentValuesShape();

    [[nodiscard]] nlohmann::json getWrittenComponentValues(
        const ComponentValues &componentValues);

    [[nodiscard]] ComponentValues getReadComponentValues(const nlohmann::json &json);

}
