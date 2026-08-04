#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/StateDump.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * What the state *is*: the campaign's memory -- which carries the
     * battle's -- and the best-score baseline the bar shows.
     * The level and the wave plan are never written; a load
     * regenerates both from the campaign's seed and the dumped level
     * index, exactly as Campaign::buildBattle() built them live.
     *
     * The policy half -- the echo lines, the refusal while recording
     * or replaying -- is console::SnapshotCommands', written once for
     * every application.
     */
    class TowerDefenceSnapshotStore final
        : public antwika::console::JsonSnapshotStore<StateDumpError>
    {
    public:
        /**
         * @brief Construct the store over the run's state.
         * @param campaign Taken by dump and rewritten by load.
         * Must outlive this store.
         * @param best The run's best-score baseline, rewritten by load
         * so a restored bar shows the record the dump was played
         * against.
         * Must outlive this store.
         */
        TowerDefenceSnapshotStore(
            Campaign &campaign, std::uint64_t &best) noexcept;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        Campaign &campaign;
        std::uint64_t &best;
    };

} // namespace antwika::tower_defence
