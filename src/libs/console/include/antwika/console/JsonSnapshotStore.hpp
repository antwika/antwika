#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/config/Format.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/SnapshotError.hpp"
#include "antwika/console/SnapshotFormat.hpp"

namespace antwika::console
{

    /**
     * @brief A snapshot store keeping its state in the shared
     * envelope, with everything but the state itself already written.
     *
     * Every application implementing ISnapshotStore repeated the same
     * three things: a SnapshotFormat built from its own magic,
     * version, title and migrations; a dump writing the console beside
     * an encoded state to a path; and a load reading that back,
     * applying it, and rewrapping whatever its own reader threw as the
     * SnapshotError this seam promises.
     * All three are here, and an application states its format to the
     * constructor, names its failure category as ErrorT, and says what
     * its state *is* through takeState() and applyState().
     *
     * **The rewrapping is deliberately narrow.** ErrorT is what the
     * application declares for its own state being wrong about
     * something, and a failure from further down -- pattern's refusal
     * to place a segment, say -- is not that and travels on as itself.
     * A store whose state's own reader already promises SnapshotError
     * names that as its ErrorT, and the rewrapping is then the
     * identity.
     *
     * What the envelope's own write throws is outside the rewrapping,
     * because it is already a SnapshotError: a full disk is the
     * machine's truth rather than the state's.
     *
     * @tparam ErrorT The failure category the application declares,
     * the one this seam turns into a SnapshotError.
     */
    template <typename ErrorT>
    class JsonSnapshotStore : public ISnapshotStore
    {
    public:
        /**
         * @brief Construct the store over the format it dumps by.
         * @param format The magic and current version, the
         * application's own. The magic must name the application --
         * e.g. "antwika-life-state-dump" -- so another one's file
         * refuses before any state is looked at.
         * @param title What the schema calls the document in a
         * refusal; a constant the caller owns.
         * @param migrations Builds the chain that brings an old dump
         * document up, exactly as every persisted format here does.
         */
        JsonSnapshotStore(
            antwika::config::Format format,
            std::string_view title,
            antwika::replay::MigrationChain (*migrations)())
            : format(format, title, migrations)
        {
        }

        JsonSnapshotStore(const JsonSnapshotStore &) = delete;
        JsonSnapshotStore(JsonSnapshotStore &&) = delete;

        JsonSnapshotStore &operator=(const JsonSnapshotStore &) =
            delete;
        JsonSnapshotStore &operator=(JsonSnapshotStore &&) = delete;

        /**
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump so
         * that coming back to the instant means reading what it read.
         * @throws SnapshotError If the state cannot be taken, or the
         * file cannot be opened or written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) final
        {
            // Named locals in assignment style, never a temporary.
            // A partly-built one needs conditional unwind cleanups.
            // Those are branches gcov counts and no input can take.
            Snapshot snapshot;
            snapshot.console = console;

            try
            {
                snapshot.state = takeState(path);
            }
            // The application's taking promises its own error.
            // What this seam promises is SnapshotError.
            catch (const ErrorT &failed)
            {
                throw SnapshotError(failed.what());
            }

            format.write(snapshot, path);
        }

        /**
         * @brief Read a file and apply the state it holds.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws SnapshotError If the file is not there, is not this
         * application's dump, or cannot be applied.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) final
        {
            auto snapshot = format.read(path);

            try
            {
                applyState(path, snapshot.state);
            }
            // The application's own reader promises its own error.
            // What this seam promises is SnapshotError.
            catch (const ErrorT &failed)
            {
                throw SnapshotError(failed.what());
            }

            return snapshot.console;
        }

    protected:
        /**
         * @brief Read the running state out, as the dump holds it.
         * @param path Where the document is going, for a store whose
         * state does not all fit in one -- atlas_editor writes its
         * bitmaps as PNGs beside the document and binds them to it by
         * fingerprint.
         * @return The state object, opaque to the envelope.
         * @throws ErrorT Whatever the application declares for its own
         * failure; it leaves dump() as a SnapshotError.
         */
        [[nodiscard]] virtual nlohmann::json takeState(
            const std::string &path) = 0;

        /**
         * @brief Make the running state the dump's.
         * @param path Where the document was read from, for the side
         * files takeState() wrote.
         * @param state The state object the dump carried, still
         * unvalidated: the envelope says only that it is an object.
         * @throws ErrorT Whatever the application declares for its own
         * failure; it leaves load() as a SnapshotError.
         */
        virtual void applyState(
            const std::string &path, const nlohmann::json &state) = 0;

    private:
        SnapshotFormat format;
    };

} // namespace antwika::console
