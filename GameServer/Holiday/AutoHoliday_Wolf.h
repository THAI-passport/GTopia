#pragma once

#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"
#include "Holiday/Holiday.h"
#include "Math/Math.h"

class AutoHoliday_Wolf : public BaseHoliday
{
public:
    AutoHoliday_Wolf(const HolidayData& data) : BaseHoliday(data) {}
    virtual ~AutoHoliday_Wolf() {}

private:
    string GetWelcomeMessage() const override
    {
        return "`3Tonight is All Howl's Eve!`` The halls of Valhowla are open to the worthy!";
    }

    void OnStarted() override
    {
        GetPlayerManager()->BroadcastMessage("`3*** It's All Howl's Eve!`` Blow a Wolf Whistle!");
    }

    void OnEnded() override
    {
        GetPlayerManager()->BroadcastMessage("`3*** All Howl's Eve has ended.`` You should probably shave.");
    }
};