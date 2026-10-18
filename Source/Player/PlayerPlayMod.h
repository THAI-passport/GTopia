#pragma once

#include "../Memory/MemoryBuffer.h"
#include "../Precompiled.h"
#include "../Utils/Timer.h"
#include "PlayModManager.h"

#define PLAYER_MOD_CONTROLLER_VERSION 1

struct ActivePlayMod
{
    ePlayModType type = PLAYMOD_TYPE_NONE;
    uint32 timeValue = 0;
    int32 extraData = 0;
    Timer updateTimer;
    Timer customTickTimer;

    uint32 GetRemainingSeconds() const
    {
        if (timeValue == 0)
            return 0;

        if (IsOnlineOnlyPlayMod(type))
            return timeValue;

        uint32 nowSec = (uint32)(Time::GetTimeSinceEpoch());
        return (nowSec < timeValue) ? (timeValue - nowSec) : 0;
    }
};

class PlayerPlayMod
{
public:
    PlayerPlayMod();
    virtual ~PlayerPlayMod();

public:
    virtual ActivePlayMod* AddPlayMod(ePlayModType type);
    virtual bool RemovePlayMod(ePlayModType type);

    uint32 GetMemEstimate();
    virtual void Serialize(MemoryBuffer& memBuffer, bool write);

    bool HasPlayMod(ePlayModType type);
    ActivePlayMod* GetActiveMod(ePlayModType type);
    uint32 GetActiveModCount() const { return m_activeMods.size(); }

protected:
    std::vector<ActivePlayMod> m_activeMods;
};

#if defined(SERVER_GAME) || defined(SERVER_MASTER)

enum ePlayerPunishType
{
    PLAYER_PUNISH_IP,
    PLAYER_PUNISH_HARD,
    PLAYER_PUNISH_SOFT,
    PLAYER_PUNISH_DEVICE
};

class DatabasePool;
void GivePlayerPlayModAndOther(DatabasePool* pPool, PlayerPlayMod& playMod, int32 playModType, int32 durationMin,
                               int32 punishType, bool overwrite, uint32 targetUserID, const string& targetName,
                               const string& targetFormattedName, uint32 adminUserID, const string& adminName,
                               const string& ip, const string& mac, const string& sid, const string& vid,
                               const string& gid, int32 hash, int32 platformType, const string& reason, bool isSilent,
                               string& statusMsg, string& broadcastMsg);

#endif