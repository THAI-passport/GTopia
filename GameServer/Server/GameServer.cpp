#include "GameServer.h"
#include "../Command/CommandManager.h"
#include "../Context.h"
#include "../Dialog/DialogManager.h"
#include "../Player/GamePlayer.h"
#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"
#include "Holiday/HolidayManager.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "MasterBroadway.h"
#include "Packet/GamePacket.h"
#include "Packet/NetPacket.h"
#include "Packet/PacketUtils.h"
#include "Player/RoleManager.h"
#include "Server/StatsManager.h"
#include "UserCacheManager.h"

#include "../Event/UDP/GameMessage/GameMessage_Input.h"
#include "../Event/UDP/GameMessage/GameMessage_Item.h"
#include "../Event/UDP/GameMessage/GameMessage_Player.h"

#include "../Holiday/AutoHoliday_Comet.h"
#include "../Holiday/AutoHoliday_Wolf.h"

static const uint32 SMALL_PACKET_SIZE = 80;
static const uint32 MED_PACKET_SIZE = 800;
static const uint32 LARGE_PACKET_SIZE = 1500;
static const uint32 HUGE_PACKET_SIZE = 400 * 1024;

static const uint32 SMALL_PACKET_COUNT = 5000;
static const uint32 MED_PACKET_COUNT = 256;
static const uint32 LARGE_PACKET_COUNT = 128;
static const uint32 HUGE_PACKET_COUNT = 10;

GameServer::GameServer() : m_serverShutdownTimeSec(-1)
{
    PacketPoolConfig cfg;
    cfg.smallPacketSize = SMALL_PACKET_SIZE;
    cfg.smallPoolSize = SMALL_PACKET_COUNT;

    cfg.medPacketSize = MED_PACKET_SIZE;
    cfg.medPoolSize = MED_PACKET_COUNT;

    cfg.largePacketSize = LARGE_PACKET_SIZE;
    cfg.largePoolSize = LARGE_PACKET_COUNT;

    cfg.hugePacketSize = HUGE_PACKET_SIZE;
    cfg.hugePoolSize = HUGE_PACKET_COUNT;

    gPacketPool.Init(cfg);
}

GameServer::~GameServer() {}

void GameServer::OnEventConnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = new GamePlayer();
    pPlayer->SetNetID(event.netID);

    GetPlayerManager()->AddPlayer(pPlayer);

    char ipBuffer[16] = {0};
    if (GetIPStringFromHost(event.host, ipBuffer, sizeof(ipBuffer)) == 0)
    {
        pPlayer->SetAddress(ipBuffer);
    }
    else
    {
        pPlayer->SetAddress("0.0.0.0");
    }

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(NetworkEvent& event)
{
    if (!event.pPacket)
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if (!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    PooledPacket* pPacket = event.pPacket;
    uint32 msgType = GetMessageTypeFromEnetPacket(pPacket->payload, pPacket->dataLength);

    switch (msgType)
    {
        case NET_MESSAGE_GENERIC_TEXT:
        case NET_MESSAGE_GAME_MESSAGE:
        {
            const char* textFromPacket = GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength);
            LOGGER_LOG_DEBUG("%s", textFromPacket);

            if (pPlayer->HasState(PLAYER_STATE_LOGIN_REQUEST))
            {
                ParsedTextPacket<40> packet;
                ParseTextPacket(textFromPacket, pPacket->dataLength - 4, packet);

                pPlayer->StartLoginRequest(packet);
            }
            else if (pPlayer->HasState(PLAYER_STATE_ENTERING_GAME))
            {
                ParsedTextPacket<38> packet;
                ParseTextPacket(textFromPacket, pPacket->dataLength - 4, packet);

                auto pAction = packet.Find("action"_hash);
                if (pAction)
                {
                    uint32 packetType = HashString(pAction->value, pAction->valueSize);

                    if (packetType == "refresh_item_data"_hash || packetType == "enter_game"_hash ||
                        packetType == "refresh_player_tribute_data"_hash || packetType == "quit"_hash)
                    {
                        m_messagePacket.Dispatch(packetType, pPlayer, packet);
                    }
                }
            }
            else if (pPlayer->HasState(PLAYER_STATE_IN_GAME))
            {
                ParsedTextPacket<38> packet;
                ParseTextPacket(textFromPacket, pPacket->dataLength - 4, packet);

                auto pAction = packet.Find("action"_hash);
                if (pAction)
                {
                    uint32 packetType = HashString(pAction->value, pAction->valueSize);
                    m_messagePacket.Dispatch(packetType, pPlayer, packet);
                }
            }

            CRASH_SET("LastTextPacket", textFromPacket);
            break;
        }

        case NET_MESSAGE_GAME_PACKET:
        {
            if (pPlayer->GetCurrentWorld() == 0)
            {
                /**
                 * response
                 */
                break;
            }

            GetWorldManager()->OnHandleGamePacket(event);
            break;
        }
    }

    gPacketPool.Release(pPacket);
}

void GameServer::OnEventDisconnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if (!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    pPlayer->LogOff(true, true, true, false);
    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());

    if (event.pPacket)
    {
        gPacketPool.Release(event.pPacket);
    }
}

void GameServer::RegisterEvents()
{
    RegisterMessagePacket<GameMessage_RefreshItemData>("refresh_item_data"_hash);
    RegisterMessagePacket<GameMessage_EnterGame>("enter_game"_hash);
    RegisterMessagePacket<GameMessage_RefreshTributeData>("refresh_player_tribute_data"_hash);
    RegisterMessagePacket<GameMessage_JoinRequest>("join_request"_hash);
    RegisterMessagePacket<GameMessage_Input>("input"_hash);
    RegisterMessagePacket<GameMessage_QuitToExit>("quit_to_exit"_hash);
    RegisterMessagePacket<GameMessage_DialogReturn>("dialog_return"_hash);
    RegisterMessagePacket<GameMessage_Trash>("trash"_hash);
    RegisterMessagePacket<GameMessage_GrowID>("growid"_hash);
    RegisterMessagePacket<GameMessage_Quit>("quit"_hash);
    RegisterMessagePacket<GameMessage_SetSkin>("setSkin"_hash);
    RegisterMessagePacket<GameMessage_Drop>("drop"_hash);
    RegisterMessagePacket<GameMessage_Wrench>("wrench"_hash);
    RegisterMessagePacket<GameMessage_Buy>("buy"_hash);
    RegisterMessagePacket<GameMessage_Store>("store"_hash);
    RegisterMessagePacket<GameMessage_Respawn>("respawn"_hash);
    RegisterMessagePacket<GameMessage_RespawnSpike>("respawn_spike"_hash);
    RegisterMessagePacket<GameMessage_Info>("info"_hash);
    RegisterMessagePacket<GameMessage_ModTrade>("mod_trade"_hash);
    RegisterMessagePacket<GameMessage_TradeLock>("trade_lock"_hash);
    RegisterMessagePacket<GameMessage_TradeAccept>("trade_accept"_hash);
    RegisterMessagePacket<Gamemessage_RemTrade>("rem_trade"_hash);
    RegisterMessagePacket<GameMessage_TradeStarted>("trade_started"_hash);
    RegisterMessagePacket<GameMessage_TradeCancel>("trade_cancel"_hash);

    GetCommandManager()->RegisterAllCommands();
    GetCommandManager()->BuildHelpCache();
    GetDialogManager()->RegisterAllDialogs();

    GetHolidayManager()->RegisterCustomHoliday<AutoHoliday_Comet>("comet");
    GetHolidayManager()->RegisterCustomHoliday<AutoHoliday_Wolf>("wolf");
}

void GameServer::UpdateGameLogic(uint64 maxTimeMS)
{
    ServerBase::UpdateGameLogic(maxTimeMS);
    GetPlayerManager()->UpdatePlayers();
    GetWorldManager()->UpdateWorlds();
    GetUserCacheManager()->Update();
    GetStatsManager()->Update();
    GetHolidayManager()->Update();

    if (m_serverShutdownTimeSec != -1 && m_serverShutdownAnnounceTimer.IsPassed())
    {
        int64 remainingMs = Max(0, (m_serverShutdownTimeSec * 1000) - (int64)m_serverShutdownTimer.GetElapsedTime());

        uint32 remainSeconds = (uint32)(remainingMs / 1000);
        uint32 remainMinutes = remainSeconds / 60;

        uint32 nextDelayAnnounceMS = 1000;
        string message = "`4Global System Message``: Restarting server for update in `4";

        if (remainMinutes > 0)
        {
            message += ToString(remainMinutes) + "`` minutes";
            nextDelayAnnounceMS = 60 * 1000;

            if (remainMinutes == 1)
            {
                nextDelayAnnounceMS = 30 * 1000;
                GetPlayerManager()->BroadcastMusic("mp3/suspended.mp3");
            }
        }
        else if (remainSeconds > 0)
        {
            message += ToString(remainSeconds) + "`` seconds";
            if (remainSeconds > 4)
                nextDelayAnnounceMS = 10 * 1000;
            else
                nextDelayAnnounceMS = 1000;
        }
        else
        {
            message += "ZERO`` seconds! Should be back up in a minute or so. BYE!";
            GetPlayerManager()->BroadcastMessage(message);

            m_serverShutdownTimeSec = -1;
            GetContext()->Shutdown();
        }

        if (remainSeconds != 0)
        {
            GetPlayerManager()->BroadcastMessage(message);
            m_serverShutdownAnnounceTimer.Set(nextDelayAnnounceMS);
        }
    }
}

void GameServer::Update()
{
    if (!m_pENetServer)
        return;

    Context* pContext = GetContext();

    uint32 currentCpuPermille = pContext->GetRuntimeStats().GetPerfStats().netCpuPermille;
    usize outgoingQueueSize = gPacketOutgoingQueue.size_approx();

    uint32 currentBurstLimit = 0;
    bool isPanicMode = false;

    EvaluateNetHealth(outgoingQueueSize, currentCpuPermille, currentBurstLimit, isPanicMode);

    if (pContext->IsShutting())
    {
        currentBurstLimit = 5000;
    }

    uint32 processedSends = 0;
    NetworkEvent outEvent;

    while (processedSends < currentBurstLimit && gPacketOutgoingQueue.try_dequeue(outEvent))
    {
        processedSends++;

        auto it = m_connectionMap.find(outEvent.netID);
        ENetPeer* pPeer = (it != m_connectionMap.end()) ? it->second : nullptr;

        if (!pPeer)
        {
            if (outEvent.pENetPacket)
                enet_packet_destroy(outEvent.pENetPacket);

            if (outEvent.pPacket)
                gPacketPool.Release(outEvent.pPacket);
            continue;
        }

        if (outEvent.shouldDisconnect)
        {
            enet_peer_disconnect(pPeer, 0);
            m_connectionMap.erase(it);

            if (outEvent.pENetPacket)
                enet_packet_destroy(outEvent.pENetPacket);

            if (outEvent.pPacket)
                gPacketPool.Release(outEvent.pPacket);
            continue;
        }

        if (outEvent.isItemData && outEvent.pPacket)
        {
            GameUpdatePacket* pGamePacket =
                GetGamePacketFromEnetPacket(outEvent.pPacket->payload, outEvent.pPacket->dataLength, false);

            if (pGamePacket && pGamePacket->type == NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA)
            {
                ItemsClientData* pClientData =
                    GetItemInfoManager()->GetClientData(pGamePacket->field_11, pGamePacket->field_10);

                if (pClientData && pClientData->pItemData)
                {
                    pGamePacket->field_10 = 0;
                    pGamePacket->field_11 = 0;

                    ENetPacket* pItemPacket = enet_packet_create(
                        nullptr, outEvent.pPacket->dataLength + pClientData->size, ENET_PACKET_FLAG_RELIABLE);
                    memcpy(pItemPacket->data, outEvent.pPacket->payload, outEvent.pPacket->dataLength);
                    memcpy(pItemPacket->data + outEvent.pPacket->dataLength - 1, pClientData->pItemData,
                           pClientData->compressSize == 0 ? pClientData->size : pClientData->compressSize);

                    enet_peer_send(pPeer, 0, pItemPacket);
                }
            }

            gPacketPool.Release(outEvent.pPacket);
        }
        else if (outEvent.pENetPacket)
        {
            if (enet_peer_send(pPeer, 0, outEvent.pENetPacket) != 0)
            {
                enet_packet_destroy(outEvent.pENetPacket);
            }
        }
    }

    if (pContext->IsShutting())
        return;

    uint32 processedReceives = 0;
    ENetEvent inEvent;

    uint32 recvLimit = isPanicMode ? gNetBurstConfig.panicBurst : currentBurstLimit;

    while (!pContext->IsShutting() && processedReceives < recvLimit && m_pENetServer->Update(&inEvent))
    {
        processedReceives++;
        if (!inEvent.peer)
            continue;

        switch (inEvent.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                uint32 newConnID = ++m_lastConnectionID;
                if (newConnID == 0)
                    newConnID = ++m_lastConnectionID;

                inEvent.peer->data = (void*)(uintptr_t)newConnID;
                m_connectionMap[newConnID] = inEvent.peer;

                NetworkEvent netEvent;
                netEvent.type = ENET_EVENT_TYPE_CONNECT;
                netEvent.netID = newConnID;
                netEvent.host = inEvent.peer->address.host;
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                if (!inEvent.packet || inEvent.packet->dataLength < 4)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                uint32 packetLen = inEvent.packet->dataLength;
                uint32 msgType = GetMessageTypeFromEnetPacket(inEvent.packet->data, packetLen);

                if (msgType != NET_MESSAGE_GAME_MESSAGE && msgType != NET_MESSAGE_GAME_PACKET &&
                    msgType != NET_MESSAGE_GENERIC_TEXT)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                if (msgType == NET_MESSAGE_GAME_PACKET && packetLen > SMALL_PACKET_SIZE)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                if ((msgType == NET_MESSAGE_GAME_MESSAGE || msgType == NET_MESSAGE_GENERIC_TEXT) &&
                    packetLen > MED_PACKET_SIZE)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                PooledPacket* pPacket = gPacketPool.Acquire(packetLen);
                if (!pPacket)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                pPacket->dataLength = packetLen;
                memcpy(pPacket->payload, inEvent.packet->data, packetLen);

                enet_packet_destroy(inEvent.packet);

                NetworkEvent netEvent{ENET_EVENT_TYPE_RECEIVE, (uint32)(uintptr_t)inEvent.peer->data, nullptr, pPacket};
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                if (!inEvent.peer)
                    continue;

                uint32 netID = (uint32)(uintptr_t)inEvent.peer->data;
                inEvent.peer->data = nullptr;

                m_connectionMap.erase(netID);

                NetworkEvent netEvent{ENET_EVENT_TYPE_DISCONNECT, netID, nullptr, nullptr};
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }
        }
    }
}

void GameServer::ForceSaveEverything()
{
    GetPlayerManager()->SaveAllToDatabase();
    GetWorldManager()->SaveAllToDatabase();
}

void GameServer::ShutdownServersAfterSeconds(int32 timeSec)
{
    if (timeSec == -1)
    {
        GetPlayerManager()->BroadcastMusic("");
    }

    m_serverShutdownTimeSec = timeSec;
    m_serverShutdownTimer.Reset();
    m_serverShutdownAnnounceTimer.Set(0);
}

void GameServer::Kill()
{
    ServerBase::Kill();

    GetItemInfoManager()->Kill();
    GetRoleManager()->Kill();
    GetWorldManager()->Kill();
    GetPlayerManager()->RemoveAllPlayers();
}

GameServer* GetGameServer()
{
    return GameServer::GetInstance();
}