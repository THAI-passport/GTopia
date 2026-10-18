#pragma once

#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"
#include "Holiday/Holiday.h"
#include "Math/Math.h"

class AutoHoliday_Locke : public BaseHoliday
{
public:
    AutoHoliday_Locke(const HolidayData& data) : BaseHoliday(data) {}
    virtual ~AutoHoliday_Locke() {}

private:
    string GetWelcomeMessage() const override
    {
        return "A `2Traveling Salesman`` has come to Growtopia! Go trade him some of your Locks for fabulous items "
               "before he leaves town again!";
    }

    void OnStarted() override
    {
        GetPlayerManager()->BroadcastMessage("`2A traveling salesman has come to Growtopia for the day!``");
    }

    void OnEnded() override
    {
        GetPlayerManager()->BroadcastMessage(
            "`2The traveling salesman has moved on!`` Hope he comes back in a couple weeks!");
    }

    void OnUpdate() override {}
};