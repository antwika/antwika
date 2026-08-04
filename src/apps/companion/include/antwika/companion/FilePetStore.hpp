#pragma once

#include <optional>
#include <string>

#include <antwika/app/FileSnapshotStore.hpp>

#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    /**
     * @brief Keeps the companion in one file.
     *
     * The only class in this application that opens anything, which is
     * what lets every other one be exercised with no file on disk.
     * It holds the path and nothing else: what a document looks like is
     * PetSave's, and both halves are split apart for the reason
     * saveGameToJson() is split from saveGameFile() -- a round trip
     * through the format is assertable with no filesystem at all.
     *
     * The path comes from the caller and is never defaulted here: which
     * file a companion lives in is the application's decision, and a
     * class that picked one would pick the same one for every session.
     */
    class FilePetStore final : public IPetStore
    {
    public:
        /**
         * @brief Construct the store over the file it uses.
         * @param path Where the companion is read from and written to.
         */
        explicit FilePetStore(std::string path);

        /**
         * @brief Read what the last session left behind.
         * @return What it was, or nothing when the file is not there --
         * a first run, which is an ordinary answer rather than a
         * failure and starts a new companion.
         * @throws SaveFormatError If the file is there and is not a
         * companion this build can read.
         */
        [[nodiscard]] std::optional<CompanionMemory> load() override;

        /**
         * @brief Write the companion and its lineage out.
         * @param memory Everything the session holds.
         * @throws SaveFormatError If the file cannot be opened, or if
         * the bytes cannot be written once it is. A companion is
         * written in one go, so failing quietly here loses the session.
         */
        void save(const CompanionMemory &memory) override;

    private:
        antwika::app::FileSnapshotStore<CompanionMemory, SaveFormatError>
            file;
    };

} // namespace antwika::companion
