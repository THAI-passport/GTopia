#pragma once

#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"
#include "Holiday/Holiday.h"
#include "Math/Math.h"

class AutoHoliday_Comet : public BaseHoliday
{
public:
    AutoHoliday_Comet(const HolidayData& data) : BaseHoliday(data) {}
    virtual ~AutoHoliday_Comet() {}

private:
    string GetWelcomeMessage() const override
    {
        return "It's the `2Night of the Comet!`` Stardust is sprinkling down from the sky, and you just might be able "
               "to get items that can only be spliced in the cosmic energy of the comet!";
    }

    void TrySpawnDust(World* pWorld)
    {
        if (!pWorld)
            return;

        Vector2Float spawnCenterPos = pWorld->GetTileManager()->GetMapStartWorldPos("");
        auto players = pWorld->GetPlayers();
        if (players.empty())
            return;

        int32 playerCountAgreed = 0;
        for (auto& pPlayer : players)
        {
            if (!pPlayer)
                continue;

            if (DistanceBetweenPoints(spawnCenterPos, pPlayer->GetWorldPosCenter()) < 128.0f)
                continue;

            playerCountAgreed++;
        }

        int32 targetDustCount = (playerCountAgreed / 2);
        if (targetDustCount == 0)
            return;

        uint32 currentDustCount =
            pWorld->GetObjectManager()->GetTotalCountOfObjectsByItemIDs({ITEM_ID_ANTIMATTER_DUST, ITEM_ID_COMET_DUST});

        if (targetDustCount > currentDustCount)
        {
            pWorld->DropRandomObjectOnGround(ITEM_ID_COMET_DUST, (targetDustCount - currentDustCount));
        }
    }

    void OnStarted() override
    {
        GetPlayerManager()->BroadcastMessage("What's that in the sky?? `2A Comet is blazing a trail through the "
                                             "night!`` It will only be here for 24 hours...");

        auto worlds = GetWorldManager()->GetWorlds();
        for (auto& [_, pWorld] : worlds)
        {
            if (pWorld)
            {
                pWorld->SendCurrentWeatherToAll();
            }
        }

        m_spawnDustTimer.Reset();
    }

    void OnEnded() override
    {
        GetPlayerManager()->BroadcastMessage("`2The Comet has passed us by!`` It should return in a month!");

        auto worlds = GetWorldManager()->GetWorlds();
        for (auto& [_, pWorld] : worlds)
        {
            if (pWorld)
            {
                pWorld->SendCurrentWeatherToAll();
            }
        }

        m_spawnDustTimer.Reset();
    }

    void OnUpdate() override
    {
        if (m_spawnDustTimer.GetElapsedTime() < (300 * 1000))
            return;

        auto worlds = GetWorldManager()->GetWorlds();

        for (auto& [_, pWorld] : worlds)
        {
            if (pWorld)
            {
                TrySpawnDust(pWorld);
            }
        }

        m_spawnDustTimer.Reset();
    }

private:
    Timer m_spawnDustTimer;
};