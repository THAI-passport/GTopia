#include "UserCacheManager.h"
#include "../Context.h"
#include "../Dialog/GameDialogs.h"
#include "../Player/GamePlayer.h"
#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"
#include "Item/ItemInfoManager.h"
#include "Player/RoleManager.h"
#include "Utils/GrowUtils.h"

UserCacheManager::UserCacheManager()
{
    m_lastCleanupTime.Reset();
}

UserCacheManager::~UserCacheManager() {}

void UserCacheManager::Init(uint32 maxExpectedPlayers)
{
    m_pendingRequests.reserve(maxExpectedPlayers);
    m_cache.reserve(maxExpectedPlayers * 2);
    m_inFlightUserIDs.reserve(32);
    m_dbWatchers.reserve(32);
}

UserMetadata* UserCacheManager::GetMetadata(uint32 userID)
{
    if (userID == 0)
        return nullptr;

    auto it = m_cache.find(userID);
    if (it != m_cache.end())
    {
        it->second.cacheTime.Reset();
        return &it->second;
    }

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(userID);
    if (pPlayer && !pPlayer->HasState(PLAYER_STATE_DELETE) && pPlayer->HasState(PLAYER_STATE_IN_GAME))
    {
        uint32 roleID = 0;
        if (Role* pRole = pPlayer->GetRole())
        {
            roleID = pRole->GetID();
        }

        UserMetadata& meta = m_cache[userID];
        meta.userID = userID;
        meta.name = pPlayer->GetRawName();
        meta.displayName = pPlayer->GetDisplayName(false);
        meta.roleID = roleID;
        meta.cacheTime.Reset();

        return &meta;
    }

    return nullptr;
}

bool UserCacheManager::IsCached(uint32 userID)
{
    return GetMetadata(userID) != nullptr;
}

void UserCacheManager::FetchMetadata(uint32 playerNetID, eCacheRequestType reqType, const std::vector<int32>& userIDs,
                                     std::array<CacheParam, 5> params, const char* textParam)
{
    if (m_pendingRequests.find(playerNetID) != m_pendingRequests.end())
        return;

    std::vector<int32> missingIDs;
    missingIDs.reserve(userIDs.size());

    for (int32 id : userIDs)
    {
        if (id > 0 && !IsCached((uint32)(id)))
        {
            missingIDs.push_back(id);
        }
    }

    if (missingIDs.empty())
    {
        PendingRequest fastRequest;
        fastRequest.reqType = reqType;
        fastRequest.params = params;
        fastRequest.SetText(textParam);
        ExecuteRequest(playerNetID, fastRequest);
        return;
    }

    std::sort(missingIDs.begin(), missingIDs.end());
    missingIDs.erase(std::unique(missingIDs.begin(), missingIDs.end()), missingIDs.end());

    PendingRequest pending;
    pending.missingCount = missingIDs.size();
    pending.reqType = reqType;
    pending.params = params;
    pending.SetText(textParam);

    m_pendingRequests.emplace(playerNetID, std::move(pending));

    for (int32 missingIDVal : missingIDs)
    {
        uint32 missingID = (uint32)missingIDVal;
        m_dbWatchers[missingID].push_back(playerNetID);

        if (m_inFlightUserIDs.find(missingID) != m_inFlightUserIDs.end())
            continue;

        m_inFlightUserIDs.insert(missingID);
        QueryRequest req = PlayerDB::GetOfflineData(missingID);
        req.callback = &UserCacheManager::OnMetadataFetched;
        req.AddExtraData(missingID);

        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
}

void UserCacheManager::OnMetadataFetched(QueryTaskResult&& result)
{
    if (result.extraData.empty())
        return;

    Variant* pFetchedUser = result.GetExtraData(0);
    if (!pFetchedUser)
        return;

    uint32 fetchedUserID = pFetchedUser->GetUINT();
    UserCacheManager* pCacheMgr = GetUserCacheManager();

    pCacheMgr->m_inFlightUserIDs.erase(fetchedUserID);

    bool userFound = false;

    if (result.status == QUERY_STATUS_OK && result.result && result.result->GetRowCount() > 0)
    {
        for (uint32 i = 0; i < result.result->GetRowCount(); ++i)
        {
            UserMetadata meta;
            meta.userID = fetchedUserID;

            const string& name = result.result->GetField("Name", i).GetString();
            const string& guestName = result.result->GetField("GuestName", i).GetString();
            int32 guestID = result.result->GetField("GuestID", i).GetINT();
            int32 roleID = result.result->GetField("RoleID", i).GetINT();

            meta.name = GetPlayerRawName(name, guestName, guestID);
            meta.displayName = GetPlayerDisplayName(meta.name, roleID, false, false, false);
            meta.cacheTime.Reset();

            pCacheMgr->m_cache[fetchedUserID] = std::move(meta);
            userFound = true;
        }
    }

    if (!userFound)
    {
        UserMetadata meta;
        meta.userID = fetchedUserID;
        meta.name = "DeletedUser";
        meta.displayName = "`4DeletedUser``";
        meta.roleID = 0;
        meta.cacheTime.Reset();

        pCacheMgr->m_cache[fetchedUserID] = std::move(meta);
    }

    auto watcherIt = pCacheMgr->m_dbWatchers.find(fetchedUserID);
    if (watcherIt != pCacheMgr->m_dbWatchers.end())
    {
        for (uint32 playerNetID : watcherIt->second)
        {
            auto reqIt = pCacheMgr->m_pendingRequests.find(playerNetID);
            if (reqIt != pCacheMgr->m_pendingRequests.end())
            {
                if (reqIt->second.missingCount > 0)
                {
                    reqIt->second.missingCount--;
                }

                if (reqIt->second.missingCount == 0)
                {
                    pCacheMgr->ExecuteRequest(playerNetID, reqIt->second);
                    pCacheMgr->m_pendingRequests.erase(reqIt);
                }
            }
        }

        pCacheMgr->m_dbWatchers.erase(watcherIt);
    }
}

void UserCacheManager::ExecuteRequest(uint32 playerNetID, const PendingRequest& request)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(playerNetID);
    if (!pPlayer)
        return;

    switch (request.reqType)
    {
        case CACHE_REQ_WORLD_LOCK_PUNCH:
        {
            OnTriedPunchedOrPlaceLockedArea(pPlayer, request);
            break;
        }

        case CACHE_REQ_WORLD_LOCK_DIALOG:
        {
            LockDialog::HandleFromCache(pPlayer, request.params[0].GetINT(), request.params[1].GetINT(),
                                        request.params[2].GetINT());
            break;
        }

        case CACHE_REQ_ACHIEVEMENT_BLOCK_PUNCH:
        {
            OnPunchedAchievementBlock(pPlayer, request);
            break;
        }

        case CACHE_REQ_MAILBOX_BLOCK:
        {
            MailboxBlockDialog::HandleFromCache(pPlayer, request.params[0].GetINT(), request.params[1].GetINT(),
                                                request.params[2].GetINT());
            break;
        }

        case CACHE_REQ_BULLETIN_BLOCK:
        {
            BulletinBlockDialog::HandleFromCache(pPlayer, request.params[0].GetINT(), request.params[1].GetINT(),
                                                 request.params[2].GetINT());
            break;
        }

        case CACHE_REQ_DONATION_BOX_BLOCK:
        {
            DonationBoxDialog::HandleFromCache(pPlayer, request.params[0].GetINT(), request.params[1].GetINT(),
                                               request.params[2].GetINT());
            break;
        }

        case CACHE_REQ_WORLD_ENTER_STAT_MESSAGE:
        {
            OnPlayerEnteredWorldStatusMessage(pPlayer, request);
            break;
        }

        default:
            break;
    }
}

void UserCacheManager::OnTriedPunchedOrPlaceLockedArea(GamePlayer* pPlayer, const PendingRequest& request)
{
    if (!pPlayer)
        return;

    int32 worldID = request.params[0].GetUINT();
    if (pPlayer->GetCurrentWorld() != worldID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(request.params[1].GetINT(), request.params[2].GetINT());
    if (!pTile)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra || pTileExtra->ownerID < 1)
        return;

    UserMetadata* pUserMeta = GetMetadata(pTileExtra->ownerID);
    if (!pUserMeta)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_LOCK)
        return;

    string notifyMsg;
    if (request.params[3].GetUINT() == 0)
    {
        notifyMsg += "`w";
        notifyMsg += pUserMeta->displayName;
        notifyMsg += "``'s `$";
        notifyMsg += pItem->name;
        notifyMsg += "``.";

        if (pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
        {
            notifyMsg += " (Open to public)";
        }
        else if (pTileExtra->HasAccess(pPlayer->GetUserID()))
        {
            notifyMsg += " (`2Access granted``)";
        }
        else
        {
            notifyMsg += " (`4No access``)";
        }
    }
    else
    {
        notifyMsg += "That area is owned by `w";
        notifyMsg += pUserMeta->displayName;
    }

    pPlayer->SendOnTalkBubble(notifyMsg, true);
    pWorld->SendPlayPositionedToAll(pPlayer, "punch_locked.wav");
}

void UserCacheManager::OnPunchedAchievementBlock(GamePlayer* pPlayer, const PendingRequest& request)
{
    if (!pPlayer)
        return;

    int32 worldID = request.params[0].GetUINT();
    if (pPlayer->GetCurrentWorld() != worldID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(request.params[1].GetINT(), request.params[2].GetINT());
    if (!pTile)
        return;

    TileExtra_Achievement* pTileExtra = pTile->GetExtra<TileExtra_Achievement>();
    if (!pTileExtra)
        return;

    UserMetadata* pUserMeta = GetMetadata(pTileExtra->ownerID);
    if (!pUserMeta)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    AchievementInfo* pAchievement = GetAchievementManager()->GetAchievement((eAchievement)(pTileExtra->achievementID));
    if (!pAchievement)
    {
        pPlayer->SendOnTalkBubble("Invalid achievement.", true);
        return;
    }

    string bubbleText;
    bubbleText += pAchievement->name;
    bubbleText += " earned by ";
    bubbleText += pUserMeta->name;

    pPlayer->SendOnTalkBubble(bubbleText, true);
}

void UserCacheManager::OnPlayerEnteredWorldStatusMessage(GamePlayer* pPlayer, const PendingRequest& request)
{
    if (!pPlayer)
        return;

    int32 worldID = request.params[0].GetUINT();
    if (pPlayer->GetCurrentWorld() != worldID)
        return;

    if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldID))
    {
        string statusMsg;
        statusMsg += "`o`5[`w";
        statusMsg += pWorld->GetWorldName();
        statusMsg += "`` `$World Locked``";

        if (UserMetadata* pUserMeta = GetMetadata(pWorld->GetWorldOwnerID()))
        {
            statusMsg += " by ";
            statusMsg += pUserMeta->displayName;
            statusMsg += "``";
        }

        if (TileInfo* pWorldLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK))
        {
            if (TileExtra_Lock* pTileExtra = pWorldLockTile->GetExtra<TileExtra_Lock>())
            {
                if (pWorldLockTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
                    statusMsg += " (`$PUBLIC``)`5]``";
                else if (pTileExtra->HasAccess(pPlayer->GetUserID()))
                    statusMsg += " (`2ACCESS GRANTED``)`5]``";
                else
                    statusMsg += "`5]``";
            }
        }

        pPlayer->SendOnConsoleMessage(statusMsg);
    }
}

void UserCacheManager::Update()
{
    if (m_lastCleanupTime.GetElapsedTime() < 300000)
        return;

    m_lastCleanupTime.Reset();

    for (auto it = m_cache.begin(); it != m_cache.end();)
    {
        if (GetPlayerManager()->GetPlayerByUserID(it->first))
        {
            ++it;
            continue;
        }

        if (it->second.cacheTime.GetElapsedTime() > 1200000)
        {
            it = m_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

UserCacheManager* GetUserCacheManager()
{
    return UserCacheManager::GetInstance();
}