#include "GamePresenceManager.h"
#include "../Player/PlayerManager.h"
#include "../Server/ServerManager.h"
#include "../World/WorldManager.h"
#include "Network/NetClient.h"

GamePresenceManager::GamePresenceManager() {}

GamePresenceManager::~GamePresenceManager() {}

PlayerPresencePacketElement GamePresenceManager::BuildPlayerElement(uint32 userID)
{
    PlayerPresencePacketElement element;
    element.userID = userID;

    if (PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID))
    {
        element.status = 1;
        element.name = pSession->name;
    }

    return element;
}

void GamePresenceManager::OnTCPPacket(NetClient* pClient, uint16 packetType, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    switch (packetType)
    {
        case TCP_PACKET_PLAYER_SUBSCRIBE:
        {
            std::vector<uint32> ids;
            if (reader.ReadArray(ids) && !ids.empty())
            {
                PlayerSubscribe(pServer->serverID, ids);
            }
            break;
        }

        case TCP_PACKET_PLAYER_UNSUBSCRIBE:
        {
            std::vector<uint32> ids;
            if (reader.ReadArray(ids) && !ids.empty())
            {
                PlayerUnSubscribe(pServer->serverID, ids);
            }
            break;
        }

        case TCP_PACKET_WORLD_UPDATE:
        {
            std::vector<WorldPresenceUpdateElement> elements;
            if (!ReadPresenceElements(reader, elements) || elements.empty())
                return;

            for (auto& elem : elements)
            {
                auto it = m_worldSubs.find(elem.worldID);
                if (it == m_worldSubs.end())
                    continue;

                WorldManager* pWorldManager = GetWorldManager();
                if (pWorldManager->IsBalancerEnabled())
                {
                    if (WorldSession* pWorldSession = pWorldManager->GetWorldByInstanceID(elem.worldID))
                    {
                        if (pWorldManager->GetBalancedWorldConfig(pWorldSession->worldName))
                        {
                            pWorldManager->WorldBalancerUpdateUserCount(pWorldSession->worldName, elem.playerCount);
                        }
                    }
                }

                it->second.info.playerCount = elem.playerCount;
                it->second.info.isSignalJammed = elem.isSignalJammed;
                m_dirtyWorlds.insert(elem.worldID);
            }
            break;
        }

        case TCP_PACKET_WORLD_REMOVE:
        {
            std::vector<WorldPresenceRemoveElement> elements;
            if (!ReadPresenceElements(reader, elements) || elements.empty())
                return;

            std::vector<WorldPresenceRemoveElement> removed;
            for (auto& elem : elements)
            {
                if (m_worldSubs.erase(elem.worldID) > 0)
                {
                    m_dirtyWorlds.erase(elem.worldID);
                    removed.push_back(elem);
                }
            }

            if (!removed.empty())
            {
                GetServerManager()->SendWorldPresenceRemoveToAll(removed);
            }
            break;
        }
    }
}

void GamePresenceManager::OnPlayerStatusChanged(uint32 userID, bool loggedOn)
{
    if (m_playerSubs.find(userID) != m_playerSubs.end())
    {
        m_dirtyUsers.insert(userID);
    }
}

void GamePresenceManager::OnWorldCreated(uint32 worldID, const string& name, uint16 serverID, uint8 worldType)
{
    WorldPresenceData& data = m_worldSubs[worldID];
    data.serverID = serverID;
    data.info.worldID = worldID;
    data.info.name = name;
    data.info.playerCount = 0;
    data.info.worldType = worldType;
    data.info.isSignalJammed = false;

    GetServerManager()->SendWorldPresenceSnapshotToAll(data.info);
}

void GamePresenceManager::OnGameServerDisconnect(uint16 serverID)
{
    std::vector<WorldPresenceRemoveElement> removedWorlds;
    for (auto it = m_worldSubs.begin(); it != m_worldSubs.end();)
    {
        if (it->second.serverID == serverID)
        {
            WorldPresenceRemoveElement rem;
            rem.worldID = it->first;
            removedWorlds.push_back(rem);

            m_dirtyWorlds.erase(it->first);
            it = m_worldSubs.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!removedWorlds.empty())
    {
        GetServerManager()->SendWorldPresenceRemoveToAll(removedWorlds);
    }

    for (auto it = m_playerSubs.begin(); it != m_playerSubs.end();)
    {
        it->second.erase(serverID);
        if (it->second.empty())
        {
            m_dirtyUsers.erase(it->first);
            it = m_playerSubs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void GamePresenceManager::OnGameServerConnected(ServerInfo* pServer)
{
    if (!pServer || !pServer->pClient || m_worldSubs.empty())
        return;

    std::vector<WorldPresenceSnapshotElement> snapshotList;
    snapshotList.reserve(m_worldSubs.size());

    for (auto& [worldID, worldData] : m_worldSubs)
    {
        snapshotList.push_back(worldData.info);
    }

    GetServerManager()->SendWorldPresenceSnapshot(pServer, snapshotList);
}

void GamePresenceManager::Update()
{
    if (!m_dirtyUsers.empty() && m_playerFlushTimer.GetElapsedTime() >= 5000)
    {
        std::unordered_map<uint16, std::vector<PlayerPresencePacketElement>> packetBatches;

        for (uint32 userID : m_dirtyUsers)
        {
            auto it = m_playerSubs.find(userID);
            if (it == m_playerSubs.end())
                continue;

            PlayerPresencePacketElement element = BuildPlayerElement(userID);
            for (uint16 serverID : it->second)
            {
                packetBatches[serverID].push_back(element);
            }
        }

        ServerManager* pServerMgr = GetServerManager();
        for (auto& [serverID, dataList] : packetBatches)
        {
            ServerInfo* pServer = pServerMgr->GetServerByID(serverID);
            if (!pServer)
                continue;

            pServerMgr->SendPlayerPresenceUpdate(pServer, dataList);
        }

        m_dirtyUsers.clear();
        m_playerFlushTimer.Reset();
    }

    if (!m_dirtyWorlds.empty() && m_worldFlushTimer.GetElapsedTime() >= 5000)
    {
        std::vector<WorldPresenceUpdateElement> broadcastVec;
        broadcastVec.reserve(m_dirtyWorlds.size());

        for (uint32 worldID : m_dirtyWorlds)
        {
            auto it = m_worldSubs.find(worldID);
            if (it == m_worldSubs.end())
                continue;

            WorldPresenceUpdateElement upd;
            upd.worldID = worldID;
            upd.playerCount = it->second.info.playerCount;
            upd.isSignalJammed = it->second.info.isSignalJammed;
            broadcastVec.push_back(upd);
        }

        if (!broadcastVec.empty())
        {
            GetServerManager()->SendWorldPresenceUpdateToAll(broadcastVec);
        }

        m_dirtyWorlds.clear();
        m_worldFlushTimer.Reset();
    }
}

void GamePresenceManager::PlayerSubscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    ServerInfo* pServer = GetServerManager()->GetServerByID(serverID);
    if (!pServer)
        return;

    std::vector<PlayerPresencePacketElement> snapshotResponse;
    snapshotResponse.reserve(userIDs.size());

    for (uint32 userID : userIDs)
    {
        m_playerSubs[userID].insert(serverID);
        snapshotResponse.push_back(BuildPlayerElement(userID));
    }

    GetServerManager()->SendPlayerPresenceSnapshot(pServer, snapshotResponse);
}

void GamePresenceManager::PlayerUnSubscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    for (uint32 userID : userIDs)
    {
        auto it = m_playerSubs.find(userID);
        if (it == m_playerSubs.end())
            continue;

        it->second.erase(serverID);
        if (it->second.empty())
        {
            m_dirtyUsers.erase(userID);
            m_playerSubs.erase(it);
        }
    }
}

GamePresenceManager* GetGamePresenceManager()
{
    return GamePresenceManager::GetInstance();
}