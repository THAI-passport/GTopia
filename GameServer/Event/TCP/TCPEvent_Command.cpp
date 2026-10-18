#include "TCPEvent_Command.h"
#include "../../Context.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/MasterBroadway.h"
#include "../../World/WorldManager.h"
#include "Player/RoleManager.h"

#ifdef ENABLE_LUA_SCRIPTING
#include "../../Lua/LuaManager.h"
#endif

void TCPEvent_Command(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    int32 commandType = 0;
    if (!reader.Read(commandType))
        return;

    switch (commandType)
    {
        case TCP_COMMAND_SETROLE:
        {
            TCPEvent_Command_SetRole(reader);
            break;
        }

        case TCP_COMMAND_GIVE_PLAYMOD:
        {
            TCPEvent_Command_GivePlayMod(reader);
            break;
        }

            /*case TCP_COMMAND_BROADCAST_MESSAGE:
            {
                TCPEvent_Command_BroadcastMessage(reader);
                break;
            }*/

        case TCP_COMMAND_LUA:
        {
            TCPEvent_Command_Lua(reader);
            break;
        }

        case TCP_COMMAND_GENERIC_RESPONSE:
        {
            uint32 senderUserID = 0;
            uint8 success = 0;
            string resultText;

            if (!reader.Read(senderUserID) || !reader.Read(success) || !reader.ReadString(resultText))
                return;

            if (senderUserID > 0)
            {
                Player* pPlayer = GetPlayerManager()->GetPlayerByUserID(senderUserID);
                if (pPlayer)
                {
                    pPlayer->SendOnConsoleMessage(resultText);
                }
            }
            else
            {
                LOGGER_LOG_INFO("[Command Response]: %s", resultText.c_str());
            }
            break;
        }
    }
}

void TCPEvent_Command_SetRole(TCPPacketReader& reader)
{
    uint32 userID = 0;
    uint32 roleID = 0;

    if (!reader.Read(userID) || !reader.Read(roleID))
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(userID);
    if (!pPlayer)
        return;

    Role* pRole = GetRoleManager()->GetRole(roleID);
    if (!pRole)
        return;

    pPlayer->SetRole(pRole);

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->SendPlayerDataConfigToAll(pPlayer);
    pWorld->SendNameChangeToAll(pPlayer);
}

void TCPEvent_Command_BroadcastMessage(TCPPacketReader& reader)
{
    string message;
    string worldName;
    string audio;

    if (!reader.ReadString(message) || !reader.ReadString(worldName) || !reader.ReadString(audio))
        return;

    GetPlayerManager()->BroadcastMessage(message, worldName, audio);
}

void TCPEvent_Command_GivePlayMod(TCPPacketReader& reader)
{
    uint64 reqID = 0;
    uint32 targetUserID = 0;
    int32 playModType = 0;
    int32 durationMin = 0;
    int32 punishType = 0;
    uint32 adminUserID = 0;
    uint8 isSilentRaw = 0;
    string reason;

    if (!reader.Read(reqID) || !reader.Read(targetUserID) || !reader.Read(playModType) || !reader.Read(durationMin) ||
        !reader.Read(punishType) || !reader.Read(adminUserID) || !reader.Read(isSilentRaw) ||
        !reader.ReadString(reason))
    {
        return;
    }

    bool isSilent = (isSilentRaw == 1);
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(targetUserID);
    if (!pPlayer)
    {
        GetMasterBroadway()->SendCommandGenericResponse(
            reqID, false,
            "Player not found on routed gameserver. Probably player logged out during request, try again.");
        return;
    }

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();

    string statusMsg, broadcastMsg;
    GivePlayerPlayModAndOther(GetContext()->GetDatabasePool(), pPlayer->GetModController(), playModType, durationMin,
                              punishType, true, pPlayer->GetUserID(), pPlayer->GetRawName(),
                              pPlayer->GetDisplayName(true), adminUserID, "", string(pPlayer->GetAddress()),
                              loginDetail.mac, loginDetail.sid, loginDetail.vid, loginDetail.gid, loginDetail.hash,
                              loginDetail.platformType, reason, isSilent, statusMsg, broadcastMsg);

    pPlayer->SaveToDatabase(true);

    if (!isSilent && !broadcastMsg.empty())
    {
        GetPlayerManager()->BroadcastMessage(broadcastMsg);
    }

    if (durationMin > 0 &&
        (playModType == PLAYMOD_TYPE_BAN || playModType == PLAYMOD_TYPE_CURSE || playModType == PLAYMOD_TYPE_DUCT_TAPE))
    {
        string warnMsg = "Warning from `4System``: You've been ";

        if (playModType == PLAYMOD_TYPE_BAN)
            warnMsg += "`4BANNED`` from `wGrowtopia`` ";
        else if (playModType == PLAYMOD_TYPE_CURSE)
            warnMsg += "`4cursed`` ";
        else if (playModType == PLAYMOD_TYPE_DUCT_TAPE)
            warnMsg += "`4duct-taped`` ";

        warnMsg += "for " + Time::ConvertTimeToStr((uint64)(durationMin) * 60 * 1000);
        pPlayer->SendOnConsoleMessage(warnMsg);
        pPlayer->SendOnAddNotification("interface/atomic_button.rttex", warnMsg, "audio/hub_open.wav", false);

        if (playModType == PLAYMOD_TYPE_BAN)
            pPlayer->LogOffWithTime(1200); // giving 1.2s for client to receive the packets
    }

    GetMasterBroadway()->SendCommandGenericResponse(reqID, true, statusMsg);
}

void TCPEvent_Command_Lua(TCPPacketReader& reader)
{
    uint64 reqID = 0;
    uint8 action = 0;
    string arg;
    uint32 number = 0;
    uint16 serverID = 0;

    if (!reader.Read(reqID) || !reader.Read(action) || !reader.ReadString(arg) || !reader.Read(number) ||
        !reader.Read(serverID))
    {
        return;
    }

    if (serverID != GetContext()->GetID())
    {
        LOGGER_LOG_WARN("TCP Command lua executed ended up in wrong server it should be %d", serverID);
        return;
    }

    bool success = false;
    string reply;

#ifdef ENABLE_LUA_SCRIPTING
    LuaManager* pLua = GetLuaManager();

    if (action == LUA_ACTION_RELOAD || action == LUA_ACTION_LOAD || action == LUA_ACTION_UNLOAD)
    {
        if (arg.empty() || arg.find('.') != string::npos || arg.find('/') != string::npos ||
            arg.find('\\') != string::npos)
        {
            reply = "Invalid script path.";
        }
    }

    switch (action)
    {
        case LUA_ACTION_RELOAD:
        {
            if (arg == "*")
            {
                uint32 total = pLua->GetScriptCount();
                uint32 ok = pLua->ReloadAll();
                success = (ok == total);
                reply = "Reloaded " + ToString(ok) + "/" + ToString(total) + " scripts" +
                        (success ? "." : ", check server log for errors.");
            }

            success = pLua->Reload(arg);
            reply = success ? "Reloaded '" + arg + "'." : "Failed to reload '" + arg + "', check server log.";
            break;
        }
        case LUA_ACTION_LOAD:
        {
            success = pLua->LoadScript(arg);
            reply = success ? "Loaded '" + arg + "'." : "Failed to load '" + arg + "', check server log.";
            break;
        }
        case LUA_ACTION_UNLOAD:
        {
            success = pLua->UnloadScript(arg);
            reply = success ? "Unloaded '" + arg + "'." : "Failed to unload '" + arg + "', check server log.";
            break;
        }
        case LUA_ACTION_PROFILE_ON:
        {
            pLua->GetProfiler().enabled = true;
            success = true;
            reply = "Profiler enabled.";
            break;
        }
        case LUA_ACTION_PROFILE_OFF:
        {
            pLua->GetProfiler().enabled = false;
            success = true;
            reply = "Profiler disabled.";
            break;
        }
        case LUA_ACTION_PROFILE_RESET:
        {
            pLua->ResetProfile();
            success = true;
            reply = "Profiler statistics reset.";
            break;
        }
        case LUA_ACTION_PROFILE_DUMP:
        {
            LuaProfileSort sort = LUA_PROFILE_SORT_TIME;
            if (arg == "time")
                sort = LUA_PROFILE_SORT_TIME;
            else if (arg == "avg")
                sort = LUA_PROFILE_SORT_AVERAGE;
            else if (arg == "max")
                sort = LUA_PROFILE_SORT_MAX;
            else if (arg == "calls")
                sort = LUA_PROFILE_SORT_CALLS;
            else if (arg == "mem")
                sort = LUA_PROFILE_SORT_MEMORY;
            else
                reply = "Unknown sort '" + arg + "'. Use time|avg|max|calls|mem";

            success = true;
            reply = "\n" + pLua->DumpProfile(sort, number);
            break;
        }
        default:
            reply = "Unknown lua action.";
    }
#else
    reply = "Lua scripting is disabled on this server.";
#endif

    GetMasterBroadway()->SendCommandGenericResponse(reqID, success, "[Server " + ToString(serverID) + "] " + reply);
}
