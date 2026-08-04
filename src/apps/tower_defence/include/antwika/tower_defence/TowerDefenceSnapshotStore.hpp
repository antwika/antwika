#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>

#include "antwika/tower_defence/Campaign.hpp"

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
        : public antwika::console::ISnapshotStore
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

        /**
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump.
         * @throws console::SnapshotError If the file cannot be
         * written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a file and apply the state it holds.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws console::SnapshotError If the file is not there, is
         * not this application's dump, or does not fit the level its
         * seed regenerates.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

    private:
        Campaign &campaign;
        std::uint64_t &best;
    };

} // namespace antwika::tower_defence
