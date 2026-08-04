#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    /**
     * @brief What every state dump of this application says it is.
     *
     * Its own magic rather than the save file's, so a companion.json
     * handed to load_state is refused as the wrong kind of file before
     * any state is looked at -- the dump embeds the save document as
     * its state member rather than being one.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-companion-state-dump";

    /**
     * @brief Which revision of the dump envelope this build writes.
     *
     * The embedded companion document polices its own version through
     * PetSave's chain, so only the envelope's shape is versioned here.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief Build the migration chain for the dump envelope.
     *
     * Empty at version 1, exactly as every persisted format here
     * starts; the companion document inside migrates through
     * standardPetMigrations() on its own terms.
     *
     * @return The chain, currently with no steps.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * console::SnapshotCommands owns the policy -- the messages, the
     * refusal while recording or replaying, the history mechanics --
     * and this owns what the state *is*: the CompanionMemory the save
     * file already round-trips, carried as the opaque state object of
     * the shared envelope under this application's own magic.
     *
     * A load rebuilds the Pet and the Lineage exactly the way
     * RestoreSink applies a companion.restore: whole, through the
     * restoring constructors, so there is no moment at which a
     * half-restored companion exists.
     */
    class CompanionSnapshotStore final
        : public antwika::console::JsonSnapshotStore<SaveFormatError>
    {
    public:
        /**
         * @brief Construct the store over the live session.
         * @param pet Remembered by a dump, replaced by a load. Must
         * outlive this store.
         * @param lineage Remembered and replaced with it. Must outlive
         * this store.
         */
        CompanionSnapshotStore(Pet &pet, Lineage &lineage) noexcept;

        CompanionSnapshotStore(const CompanionSnapshotStore &) = delete;
        CompanionSnapshotStore(CompanionSnapshotStore &&) = delete;

        CompanionSnapshotStore &operator=(
            const CompanionSnapshotStore &) = delete;
        CompanionSnapshotStore &operator=(
            CompanionSnapshotStore &&) = delete;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        Pet &pet;
        Lineage &lineage;
    };

} // namespace antwika::companion
