#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/console/SnapshotError.hpp"

namespace antwika::console
{

    /**
     * @brief One application's whole state, as a dump file holds it.
     *
     * The envelope is this library's and identical across every app
     * -- the console's history beside one opaque state object -- while
     * the magic, the version, the migrations and what the state
     * member *means* are each application's own.
     * Loading one app's dump into another therefore refuses on the
     * magic before any state is looked at.
     */
    struct Snapshot
    {
        /** @brief The console's history, oldest line first. */
        std::vector<std::string> console;

        /**
         * @brief The application's state, encoded by its own store.
         *
         * Opaque here: the envelope's schema says only that it is an
         * object, and the application validates and decodes it behind
         * ISnapshotStore.
         */
        nlohmann::json state = nlohmann::json::object();

        /**
         * @brief Compare two snapshots.
         * @param other The snapshot to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const Snapshot &other) const = default;
    };

    /**
     * @brief One application's dump-file format: the shared envelope
     * under that application's magic, version and migrations.
     */
    class SnapshotFormat final
    {
    public:
        /**
         * @brief Construct the format an application dumps by.
         * @param format The magic and current version, the app's own.
         * The magic must name the application -- e.g.
         * "antwika-life-state-dump" -- so another app's file refuses.
         * @param title What the schema calls the document in a
         * refusal; a constant the caller owns.
         * @param migrations Builds the chain that brings an old dump
         * document up, exactly as every persisted format here does.
         */
        SnapshotFormat(
            antwika::config::Format format,
            std::string_view title,
            antwika::replay::MigrationChain (*migrations)());

        /**
         * @brief Encode a snapshot as JSON matching the envelope.
         * @param snapshot The state to encode.
         * @return The encoded document, stamped with the format.
         */
        [[nodiscard]] nlohmann::json toJson(
            const Snapshot &snapshot) const;

        /**
         * @brief Decode a snapshot from JSON, validating it first.
         * @param document The document to read.
         * @return The decoded snapshot, its state still opaque.
         * @throws SnapshotError If the document is not this
         * application's dump at a version this build reads.
         */
        [[nodiscard]] Snapshot fromJson(
            const nlohmann::json &document) const;

        /**
         * @brief Write a snapshot to a file.
         * @param snapshot The state to write.
         * @param path Where to write it.
         * @throws SnapshotError If the file cannot be opened, or the
         * bytes cannot be written once it is.
         */
        void write(
            const Snapshot &snapshot, const std::string &path) const;

        /**
         * @brief Read a snapshot from a file.
         * @param path The file to read.
         * @return The decoded snapshot.
         * @throws SnapshotError If the file cannot be opened at all,
         * or does not hold this application's dump.
         */
        [[nodiscard]] Snapshot read(const std::string &path) const;

    private:
        antwika::config::FileFormat<Snapshot, SnapshotError> format;
    };

} // namespace antwika::console
