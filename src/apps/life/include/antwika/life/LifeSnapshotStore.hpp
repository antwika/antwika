#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/StateDump.hpp"

namespace antwika::life
{

    class LifeSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<StateDumpError>
    {
    public:
        LifeSnapshotStore(
            World &world,
            const Grid &grid,
            DragState &drag,
            std::optional<std::reference_wrapper<PointerToggleSink>>
                pointer) noexcept;

        LifeSnapshotStore(const LifeSnapshotStore &) = delete;
        LifeSnapshotStore(LifeSnapshotStore &&) = delete;

        LifeSnapshotStore &operator=(const LifeSnapshotStore &) = delete;
        LifeSnapshotStore &operator=(LifeSnapshotStore &&) = delete;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        [[nodiscard]] StateDump take() const;

        void apply(const StateDump &dump);

        World &world;
        const Grid &grid;
        DragState &drag;
        std::optional<std::reference_wrapper<PointerToggleSink>> pointer;
    };

}
