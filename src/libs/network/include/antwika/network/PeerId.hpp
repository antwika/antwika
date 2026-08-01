#pragma once

#include <cstdint>

namespace antwika::network
{

    /**
     * @brief One peer, as named by the host that is talking to it.
     *
     * **A peer's name is local to the host that assigned it**, exactly
     * as a socket is: the id host A holds for host B is not the id B
     * holds for A, and neither of them travels anywhere.
     * An identity a whole session agrees on is something peers reach by
     * talking to each other, so it belongs to whatever lays a session
     * over this rather than to a transport that has only ever seen one
     * end of a link.
     *
     * Ids are handed out in ascending order and never reused, so a
     * stale one can only ever name a peer that has gone rather than
     * somebody else -- the guarantee ecs::EntityManager makes, kept
     * here for the reason it makes it.
     */
    enum class PeerId : std::uint32_t
    {
    };

    /**
     * @brief Get the number behind a peer id.
     * @param peer The peer to read.
     * @return Its raw value.
     */
    [[nodiscard]] constexpr std::uint32_t rawValue(PeerId peer) noexcept
    {
        return static_cast<std::uint32_t>(peer);
    }

} // namespace antwika::network
