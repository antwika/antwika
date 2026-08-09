#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class CampaignSink final : public ITickEventSink
    {
    public:
        explicit CampaignSink(Campaign &campaign);

        CampaignSink(const CampaignSink &) = delete;
        CampaignSink(CampaignSink &&) = delete;

        CampaignSink &operator=(const CampaignSink &) = delete;
        CampaignSink &operator=(CampaignSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Campaign &campaign;
    };

}
