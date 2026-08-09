#pragma once

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>

#include "antwika/atlas_editor/EditorState.hpp"

namespace antwika::atlas_editor
{

    class EditorSnapshotStore final
        : public console::IJsonSnapshotStore<std::runtime_error>
    {
    public:
        explicit EditorSnapshotStore(EditorState &state) noexcept;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &dumped) override;

        EditorState &state;
    };

}
