#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Steps the campaign once per engine.tick.
     *
     * One engine tick is one step of the campaign, the way one tick is
     * one step of the poker loop.
     * It defines no event of its own: a step is regenerated from the
     * tick, so which wave came out, what it was made of, what died and
     * what it was worth are none of them ever persisted.
     */
    class CampaignSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the campaign it steps.
         * @param campaign Stepped once per tick. Must outlive this sink.
         */
        explicit CampaignSink(Campaign &campaign);

        CampaignSink(const CampaignSink &) = delete;
        CampaignSink(CampaignSink &&) = delete;

        CampaignSink &operator=(const CampaignSink &) = delete;
        CampaignSink &operator=(CampaignSink &&) = delete;

        /**
         * @brief Step the campaign if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         * @throws LevelError If a cleared level's successor cannot be
         * generated.
         */
        void handle(const TickEvent &event) override;

    private:
        Campaign &campaign;
    };

} // namespace antwika::tower_defence
