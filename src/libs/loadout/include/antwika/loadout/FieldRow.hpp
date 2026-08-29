#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/FieldKind.hpp>

namespace antwika::loadout
{

    struct FieldRow final
    {
        std::string_view key;

        FieldKind kind;

        double least;

        double most;

        nlohmann::json (*valueOf)(const ComponentValue &);

        void (*setFrom)(ComponentValue &, const nlohmann::json &);

        std::string (*textOf)(const ComponentValue &);

        bool (*setFromText)(ComponentValue &, std::string_view);
    };

}
