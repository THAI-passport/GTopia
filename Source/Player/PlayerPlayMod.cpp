#include "PlayerPlayMod.h"

PlayerPlayMod::PlayerPlayMod() {}

PlayerPlayMod::~PlayerPlayMod() {}

ActivePlayMod* PlayerPlayMod::AddPlayMod(ePlayModType type)
{
    if (type == PLAYMOD_TYPE_NONE || m_activeMods.size() >= 50)
        return nullptr;

    PlayMod* pConfig = GetPlayModManager()->GetPlayMod(type);
    if (!pConfig)
        return nullptr;

    bool isOnlineOnly = IsOnlineOnlyPlayMod(type);
    uint32 durationSec = pConfig->GetTime();

    if (ActivePlayMod* pExist = GetActiveMod(type))
    {
        if (isOnlineOnly)
        {
            pExist->timeValue = durationSec;
        }
        else
        {
            pExist->timeValue = (durationSec > 0) ? ((uint32)(Time::GetTimeSinceEpoch()) + durationSec) : 0;
        }

        pExist->updateTimer.Reset();
        return pExist;
    }

    ActivePlayMod newMod;
    newMod.type = type;

    if (isOnlineOnly)
    {
        newMod.timeValue = durationSec;
    }
    else
    {
        newMod.timeValue = (durationSec > 0) ? ((uint32)(Time::GetTimeSinceEpoch()) + durationSec) : 0;
    }

    newMod.updateTimer.Reset();
    newMod.customTickTimer.Reset();

    m_activeMods.push_back(std::move(newMod));
    return &m_activeMods.back();
}

bool PlayerPlayMod::RemovePlayMod(ePlayModType type)
{
    for (auto it = m_activeMods.begin(); it != m_activeMods.end(); ++it)
    {
        if (it->type == type)
        {
            if (&(*it) != &m_activeMods.back())
            {
                *it = std::move(m_activeMods.back());
            }
            m_activeMods.pop_back();
            return true;
        }
    }

    return false;
}

uint32 PlayerPlayMod::GetMemEstimate()
{
    return sizeof(uint16) * 2 + (m_activeMods.size() * (sizeof(uint16) + sizeof(uint32) + sizeof(int32)));
}

void PlayerPlayMod::Serialize(MemoryBuffer& memBuffer, bool write)
{
    uint16 version = PLAYER_MOD_CONTROLLER_VERSION;
    memBuffer.ReadWrite(version, write);

    uint16 modCount = m_activeMods.size();
    memBuffer.ReadWrite(modCount, write);

    if (!write)
    {
        m_activeMods.resize(modCount);
    }

    for (uint16 i = 0; i < modCount; ++i)
    {
        ActivePlayMod& activeMod = m_activeMods[i];
        uint16 rawType = (uint16)(activeMod.type);
        memBuffer.ReadWrite(rawType, write);
        if (!write)
        {
            activeMod.type = (ePlayModType)(rawType);
        }

        memBuffer.ReadWrite(activeMod.timeValue, write);
        memBuffer.ReadWrite(activeMod.extraData, write);
    }
}

bool PlayerPlayMod::HasPlayMod(ePlayModType type)
{
    for (auto& mod : m_activeMods)
    {
        if (mod.type == type)
            return true;
    }

    return false;
}

ActivePlayMod* PlayerPlayMod::GetActiveMod(ePlayModType type)
{
    for (auto& mod : m_activeMods)
    {
        if (mod.type == type)
            return &mod;
    }
    return nullptr;
}

#if defined(SERVER_GAME) || defined(SERVER_MASTER)
#include "../Database/Table/BanDBTable.h"
#include "../Utils/StringUtils.h"

void GivePlayerPlayModAndOther(DatabasePool* pPool, PlayerPlayMod& playMod, int32 playModType, int32 durationMin,
                               int32 punishType, bool overwrite, uint32 targetUserID, const string& targetName,
                               const string& targetFormattedName, uint32 adminUserID, const string& adminName,
                               const string& ip, const string& mac, const string& sid, const string& vid,
                               const string& gid, int32 hash, int32 platformType, const string& reason, bool isSilent,
                               string& statusMsg, string& broadcastMsg)
{
    if (!pPool || playModType == PLAYMOD_TYPE_NONE)
    {
        statusMsg = "Invalid parameters or database pool.";
        return;
    }

    ePlayModType modEnum = (ePlayModType)playModType;

    if (durationMin <= 0)
    {
        if (playMod.HasPlayMod(modEnum))
        {
            playMod.RemovePlayMod(modEnum);

            if (modEnum == PLAYMOD_TYPE_BAN)
            {
                DatabaseBanRemoveExec(pPool, playModType, mac, gid, sid, vid, ip, QueryRequest{0});
            }

            statusMsg = "Removed PlayMod " + ToString(playModType) + " from " + targetName;
        }
        else
        {
            statusMsg = "PlayMod " + ToString(playModType) + " was not active for " + targetName;
        }
        return;
    }

    bool isAlreadyActive = playMod.HasPlayMod(modEnum);
    if (isAlreadyActive && !overwrite)
    {
        statusMsg = "PlayMod " + ToString(playModType) + " is already active on " + targetName;
        return;
    }

    if (ActivePlayMod* pActiveMod = playMod.AddPlayMod(modEnum))
    {
        if (IsOnlineOnlyPlayMod(modEnum))
        {
            pActiveMod->timeValue = (uint32)(durationMin * 60);
        }
        else
        {
            pActiveMod->timeValue = (uint32)(Time::GetTimeSinceEpoch()) + (uint32)(durationMin * 60);
        }
    }

    if (!isSilent &&
        (modEnum == PLAYMOD_TYPE_BAN || modEnum == PLAYMOD_TYPE_CURSE || modEnum == PLAYMOD_TYPE_DUCT_TAPE))
    {
        string formattedName =
            targetFormattedName.empty() ? (targetName.empty() ? "<HIDDEN>" : targetName) : targetFormattedName;

        broadcastMsg = "`#**`` `$The Ancients Ones`` have ";

        if (modEnum == PLAYMOD_TYPE_BAN)
            broadcastMsg += "`4banned`` " + formattedName;
        else if (modEnum == PLAYMOD_TYPE_CURSE)
            broadcastMsg += "`bcursed`` " + formattedName + " ``to the netherworld";
        else if (modEnum == PLAYMOD_TYPE_DUCT_TAPE)
            broadcastMsg += "`bduct-taped`` " + formattedName + "'s mouth";

        broadcastMsg += " `#**`` (`4/rules`` to see the rules!)";
    }

    if (punishType != PLAYER_PUNISH_SOFT &&
        (modEnum == PLAYMOD_TYPE_BAN || modEnum == PLAYMOD_TYPE_CURSE || modEnum == PLAYMOD_TYPE_DUCT_TAPE))
    {
        DatabaseBanAddExec(pPool, targetUserID, adminUserID, punishType, playModType, durationMin, reason, ip, hash,
                           platformType, mac, gid, sid, vid, QueryRequest{0});
    }

    statusMsg = "Applied PlayMod " + ToString(playModType) + " to " + targetName + " for " + ToString(durationMin) +
                " minutes.";
}

#endif