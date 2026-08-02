#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Puts the companion a companion.restore event carries in
     * place of the one the session started with.
     *
     * **The one road back in from a file, and a live run takes it too.**
     * A session always begins on a brand new companion and is restored
     * by this sink, whether the event came from `RestoreSource` reading
     * a store or from a recording being replayed -- so a live run and
     * the replay of it reach the same animal through the same code, by
     * construction rather than by the two happening to agree.
     *
     * Registered first, so a press dispatched on the very tick the
     * companion arrives on lands on the restored one rather than on the
     * new one it displaced.
     *
     * It defines no event of its own and adds nothing to the stream: it
     * only ever reads one somebody upstream of the recorder put there.
     */
    class RestoreSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over what it replaces.
         * @param pet Replaced by whatever the event carries. Must
         * outlive this sink.
         * @param lineage Replaced with it, since a file remembers both
         * and they have different lifetimes. Must outlive this sink.
         */
        RestoreSink(Pet &pet, Lineage &lineage);

        RestoreSink(const RestoreSink &) = delete;
        RestoreSink(RestoreSink &&) = delete;

        RestoreSink &operator=(const RestoreSink &) = delete;
        RestoreSink &operator=(RestoreSink &&) = delete;

        /**
         * @brief Restore the companion if this is a restore.
         * @param event The event to fold in; anything but
         * companion.restore is ignored.
         * @throws SaveFormatError If the payload is not a companion
         * document this build can read, or describes a companion no
         * session could be in -- a recording naming one is as broken as
         * a file naming one, and is refused on the same terms.
         */
        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        Lineage &lineage;
    };

} // namespace antwika::companion
