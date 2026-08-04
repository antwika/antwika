#pragma once

#include <string>
#include <vector>

namespace antwika::console
{

    /**
     * @brief Where an application's whole state goes and comes from.
     *
     * The application's half of dump_state and load_state: what its
     * state *is*, how it is encoded and how it is applied are its own,
     * behind this seam, while the commands' policy -- the messages,
     * the refusal while recording or replaying, the history mechanics
     * -- is SnapshotCommands' and written once.
     *
     * An implementation reads and writes the SnapshotFormat envelope,
     * so every app's dump file carries its console alongside its
     * state and refuses another app's dump by magic alone.
     */
    class ISnapshotStore
    {
    public:
        virtual ~ISnapshotStore() = default;

        /**
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump so
         * that coming back to the instant means reading what it read.
         * @throws SnapshotError If the file cannot be written.
         * A full disk is the machine's truth rather than the run's, so
         * the caller lets it end the run instead of becoming a history
         * line a replay would then disagree about.
         */
        virtual void dump(
            const std::string &path,
            const std::vector<std::string> &console) = 0;

        /**
         * @brief Read a file and apply the state it holds.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws SnapshotError If the file is not there, is not this
         * application's dump, or cannot be applied.
         */
        [[nodiscard]] virtual std::vector<std::string> load(
            const std::string &path) = 0;
    };

} // namespace antwika::console
