#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/TableMemory.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/poker/PrinterMemory.hpp"

namespace antwika::poker
{

    /**
     * @brief What every dump of this application says it is.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-poker-state-dump";

    /**
     * @brief The dump revision this build writes.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief A dump-format failure of this application's own.
     */
    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief The running room, as the console's dump_state takes it.
     *
     * **The table, the deck, the generator's counter, the money and
     * the narration** -- the five things a mid-hand instant is made
     * of.
     * The generator's counter and the deck's order are both carried
     * because neither derives the other: the shuffle that produced
     * the order has already drawn, and the counter is what the *next*
     * shuffle draws from.
     *
     * What it deliberately does not carry: the agents, which are
     * stateless policy rebuilt from config; and the engine's tick
     * number, which a recording forbids going backwards.
     */
    struct RoomDump
    {
        /** @brief The generator's counter, mid-stream. */
        std::uint64_t bits = 0;

        /** @brief The deck's order and deal cursor. */
        antwika::holdem::DeckMemory deck;

        /** @brief The table's whole standing, mid-hand included. */
        antwika::holdem::TableMemory table;

        /** @brief Every bankroll, by player name. */
        std::map<std::string, antwika::holdem::Chips> balances;

        /** @brief Who sits where, one name per seat. */
        std::vector<std::string> names;

        /** @brief The hand history's mid-hand standing. */
        PrinterMemory printer;

        /**
         * @brief Compare two dumps.
         * @param other The dump to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const RoomDump &other) const = default;
    };

    /**
     * @brief Build the chain that brings an old dump document up.
     * @return The chain, currently with no steps.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as the envelope's opaque state object.
     * @param dump The state to encode.
     * @return The state object, magic-free -- the envelope stamps the
     * document; see console::SnapshotFormat.
     */
    [[nodiscard]] nlohmann::json roomDumpToJson(const RoomDump &dump);

    /**
     * @brief Decode the envelope's state object, validating it first.
     * @param state The state object a snapshot carried.
     * @return The decoded state.
     * @throws StateDumpError If the object is not a state this build
     * can read.
     */
    [[nodiscard]] RoomDump roomDumpFromJson(const nlohmann::json &state);

} // namespace antwika::poker
