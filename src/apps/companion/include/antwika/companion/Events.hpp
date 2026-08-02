#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 *
 * One, and it carries the one thing here that came from outside the
 * program: the companion a file on the machine was holding.
 * **Nothing this application can work out again is ever an event of its
 * own**, which is why there is no `companion.fed`, no `companion.slept`
 * and no `companion.perished` -- a press is recorded, and which prop it
 * landed on and whether the companion was in a state to answer are
 * regenerated on replay from the same press against the same layout.
 */
namespace antwika::companion::events
{

    /**
     * @brief Starts a session on the companion a file remembered.
     *
     * The payload is the very document `FilePetStore` writes -- magic,
     * version and all -- so one codec reads both, and a companion saved
     * by an older build is migrated on its way into a replay exactly as
     * it is on its way in from disk.
     *
     * It is here rather than a constructor argument for `PuzzleSource`'s
     * reason, which is the rule this application broke before it
     * existed: a live `--record` run began from the companion on the
     * machine that recorded it, while the recording held only the
     * presses -- so replaying that file started from a *new* companion,
     * and every press meant something else. What a prop press does
     * depends entirely on the state it lands on: a bowl offered to a
     * companion that is not hungry is a prod, a bed is refused above the
     * tired mark, and the "new pet" button only exists once one has
     * perished.
     *
     * A replay adds none of its own: the recording already carries this
     * event, and a second one would replace the companion halfway
     * through the session that was recorded on it.
     */
    inline constexpr const char *kRestore = "companion.restore";

} // namespace antwika::companion::events
