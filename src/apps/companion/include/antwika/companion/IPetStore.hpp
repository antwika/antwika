#pragma once

#include <optional>

#include "antwika/companion/CompanionMemory.hpp"

namespace antwika::companion
{

    /**
     * @brief Where the companion is kept between one session and the
     * next.
     *
     * The one seam between this application and a filesystem, in
     * atlas_editor::IAtlasStore's shape and for its reason: every other
     * class here is exercised with no file on disk at all, because a
     * test hands the session a store that answers from memory and the
     * session cannot tell the difference.
     *
     * It carries a CompanionMemory rather than a Pet, so nothing that
     * reads or writes a file can reach the live companion -- the same
     * split SaveGame keeps between the value a file holds and the
     * session it came from.
     */
    class IPetStore
    {
    public:
        virtual ~IPetStore() = default;

        /**
         * @brief Read what the last session left behind.
         * @return What it was, or nothing when there is no previous
         * companion -- a first run, or a store given nowhere to look.
         * That is an ordinary answer and starts a new companion.
         * @throws SaveFormatError If there is something to read and it
         * is not a companion this build can read.
         */
        [[nodiscard]] virtual std::optional<CompanionMemory> load() = 0;

        /**
         * @brief Write the companion and its lineage out.
         * @param memory Everything the session holds.
         * @throws SaveFormatError If the bytes cannot be written.
         */
        virtual void save(const CompanionMemory &memory) = 0;
    };

} // namespace antwika::companion
