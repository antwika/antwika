#include "antwika/tower_defence/CampaignSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::tower_defence
{

    CampaignSink::CampaignSink(Campaign &campaign) : campaign(campaign)
    {
    }

    void CampaignSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        campaign.step();
    }

}
