#ifdef ENABLE_LUA_SCRIPTING

#include "PlayerBinding.h"
#include "../../Player/GamePlayer.h"
#include "../../Player/PlayerManager.h"
#include "../LuaManager.h"

void RegisterPlayerBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    auto gamePlayer = pLuaManager->BindClass<GamePlayer>("GamePlayer");
    gamePlayer.DefMethod("hasState", &GamePlayer::HasState)
        .DefMethod("setFlag", &GamePlayer::SetFlag)
        .DefMethod("hasFlag", &GamePlayer::HasFlag)
        .DefMethod("saveToDatabase", &GamePlayer::SaveToDatabase)
        .DefMethodDefault("logOff", &GamePlayer::LogOff, (bool)true)
        .DefMethod("hasGrowId", &GamePlayer::HasGrowID)
        .DefMethod("randomizeNextDbSaveTime", &GamePlayer::RandomizeNextDBSaveTime)
        .DefMethod("getNextDbSaveTime", &GamePlayer::GetNextDBSaveTime)
        .DefMethod("getWorldVersion", &GamePlayer::GetWorldVersion)
        .DefMethodDefault("setTargetJoinWorld", &GamePlayer::SetTargetJoinWorld, string(""))
        .DefMethod("sendPositionToWorldPlayers", &GamePlayer::SendPositionToWorldPlayers)
        .DefMethod("isJoiningWorld", &GamePlayer::IsJoiningWorld)
        .DefMethod("getCurrentWorld", &GamePlayer::GetCurrentWorld)
        .DefMethod("setWorldPos", &GamePlayer::SetWorldPos)
        .DefMethod("getWorldPosCenter", &GamePlayer::GetWorldPosCenter)
        .DefMethod("getPlayerWorldRect", &GamePlayer::GetPlayerWorldRect)
        .DefMethod("setRespawnPos", &GamePlayer::SetRespawnPos)
        .DefMethod("getDisplayName", &GamePlayer::GetDisplayName)
        .DefMethod("getRawName", &GamePlayer::GetRawName)
        .DefMethod("getSpawnData", &GamePlayer::GetSpawnData)
        .DefMethod("getCountryData", &GamePlayer::GetCountryData)
        .DefMethod("setSearchName", &GamePlayer::SetSearchName)
        .DefMethod("getSearchName", &GamePlayer::GetSearchName)
        .DefMethod("toggleCloth", &GamePlayer::ToggleCloth)
        .DefMethod("toggleBattlePetLeash", &GamePlayer::ToggleBattlePetLeash)
        .DefMethod("getActiveBattlePetSlot", &GamePlayer::GetActiveBattlePetSlot)
        .DefMethod("modifyInventoryItem", &GamePlayer::ModifyInventoryItem)
        .DefMethod("trashItem", &GamePlayer::TrashItem)
        .DefMethod("dropItem", &GamePlayer::DropItem)
        .DefMethod("canActivateItemNow", &GamePlayer::CanActivateItemNow)
        .DefMethod("resetItemActiveTime", &GamePlayer::ResetItemActiveTime)
        .DefMethod("setGems", &GamePlayer::SetGems)
        .DefMethod("getGems", &GamePlayer::GetGems)
        .DefMethod("sendGems", &GamePlayer::SendGems)
        .DefMethod("modifyGems", &GamePlayer::ModifyGems)
        .DefMethod("giveXp", &GamePlayer::GiveXP)
        .DefMethod("getPlayerLevel", &GamePlayer::GetPlayerLevel)
        .DefMethod("getPlayerNextLevelXp", &GamePlayer::GetPlayerNextLevelXP)
        .DefMethod("normalizeSkinColor", &GamePlayer::NormalizeSkinColor)
        .DefMethod("sendLockAccessRequest", &GamePlayer::SendLockAccessRequest)
        .DefMethod("setLockAccessTile", &GamePlayer::SetLockAccessTile)
        .DefMethod("getLockAccessOwnerId", &GamePlayer::GetLockAcessOwnerID)
        .DefMethod("acceptLockAccess", &GamePlayer::AcceptLockAccess)
        .DefMethod("getDistToTile", &GamePlayer::GetDistToTile)
        .DefMethod("getDistToTileInTiles", &GamePlayer::GetDistToTileInTiles)
        .DefMethod("hasLosToTile", &GamePlayer::HasLOSToTile)
        .DefMethod("ableToWorldKickOrPullSomeone", &GamePlayer::AbleToWorldKickOrPullSomeone)
        .DefMethod("ableToWorldBanSomeone", &GamePlayer::AbleToWorldBanSomeone)
        .DefMethod("onDeath", &GamePlayer::OnDeath)
        .DefMethodDefault("onDeathSpike", &GamePlayer::OnDeathSpike, (int32)-1, (int32)-1)

        .DefMethod("getInventory", &GamePlayer::GetInventory)
        .DefMethod("getLoginDetail", &GamePlayer::GetLoginDetail)
        .DefMethod("getModController", &GamePlayer::GetModController)
        .DefMethod("getProgress", &GamePlayer::GetProgressData)
        .DefMethod("getExtraData", &GamePlayer::GetExtraData)

        // Player
        .DefMethod("sendHelloPacket", &Player::SendHelloPacket)
        .DefMethod("sendLogonFailWithLog", &Player::SendLogonFailWithLog)
        .DefMethod("sendWelcomePacket", &Player::SendWelcomePacket)
        .DefMethod("sendOnSendToServer", &Player::SendOnSendToServer)
        .DefMethod("sendOnConsoleMessage", &Player::SendOnConsoleMessage)
        .DefMethod("sendOnRequestWorldSelectMenu", &Player::SendOnRequestWorldSelectMenu)
        .DefMethod("sendOnFailedToEnterWorld", &Player::SendOnFailedToEnterWorld)
        .DefMethod("sendOnSpawn", &Player::SendOnSpawn)
        .DefMethodDefault("sendOnChangeSkin", &Player::SendOnChangeSkin, (Player*)nullptr)
        .DefMethodDefault("sendOnTalkBubble", &Player::SendOnTalkBubble, (Player*)nullptr)
        .DefMethod("sendOnSetCurrentWeather", &Player::SendOnSetCurrentWeather)
        .DefMethod("sendOnRemove", &Player::SendOnRemove)
        .DefMethodDefault("sendOnDialogRequest", &Player::SendOnDialogRequest, false)
        .DefMethod("sendOnTextOverlay", &Player::SendOnTextOverlay)
        .DefMethodDefault("sendOnPlayPositioned", &Player::SendOnPlayPositioned, (Player*)nullptr)
        .DefMethod("sendOnNameChanged", &Player::SendOnNameChanged)
        .DefMethod("sendOnSetBux", &Player::SendOnSetBux)
        .DefMethodDefault("sendOnDataConfig", &Player::SendOnDataConfig, (Player*)nullptr)
        .DefMethodDefault("sendOnParticleEffect", &Player::SendOnParticleEffect, (int32)0, (float)0.0f)
        .DefMethod("sendOnStoreRequest", &Player::SendOnStoreRequest)
        .DefMethod("sendOnStorePurchaseResult", &Player::SendOnStorePurchaseResult)
        .DefMethodDefault("sendOnAction", &Player::SendOnAction, (Player*)nullptr)
        .DefMethod("sendOnAddNotification", &Player::SendOnAddNotification)
        .DefMethod("sendOnSetFeatureEnableFlags", &Player::SendOnSetFeatureEnableFlags)
        .DefMethod("sendOnSetFreezeState", &Player::SendOnSetFreezeState)
        .DefMethodDefault("sendOnZoomCamera", &Player::SendOnZoomCamera, (int32)1000)
        .DefMethod("sendOnCountryState", &Player::SendOnCountryState)
        .DefMethod("sendOnStartTrade", &Player::SendOnStartTrade)
        .DefMethod("sendOnTradeStatus", &Player::SendOnTradeStatus)
        .DefMethodDefault("sendOnBillboardChange", &Player::SendOnBillboardChange, (bool)false)
        .DefMethod("sendOnForceTradeEnded", &Player::SendOnForceTradeEnded)
        .DefMethod("sendOnKilled", &Player::SendOnKilled)
        .DefMethod("sendFakePingReply", &Player::SendFakePingReply)
        .DefMethodDefault("playSfx", &Player::PlaySFX, (int32)-1)
        .DefMethod("getNetId", &Player::GetNetID)
        .DefMethod("getUserId", &Player::GetUserID)
        .DefMethod("getAddressNum", &Player::GetAddressNum)
        .DefMethod("getLastAction", &Player::GetLastAction)
        .DefMethod("resetFeatures", &Player::ResetFeatures)
        .DefMethod("enableFeature", &Player::EnableFeature)
        .DefMethod("disableFeature", &Player::DisableFeature)
        .DefMethod("isFeatureEnabled", &Player::IsFeatureEnabled)
        .DefMethod("buildFeaturesBase64String", &Player::BuildFeaturesBase64String)
        .DefMethod("sendInventoryPacket", &Player::SendInventoryPacket)
        .DefMethod("closePaginatedDialog", &Player::ClosePaginatedDialog)
        .DefMethodDefault("sendOnSetClothing", &Player::SendOnSetClothing, (Player*)nullptr)
        .DefMethod("sendOnPlanterActivated", &Player::SendOnPlanterActivated)
        .DefMethodDefault("sendCharacterState", &Player::SendCharacterState, (Player*)nullptr)
        .DefMethodDefault("sendOnSetPos", &Player::SendOnSetPos, (Player*)nullptr);

    auto loginDetail = pLuaManager->BindClass<PlayerLoginDetail>("PlayerLoginDetail");
    loginDetail.DefMember("requestedName", &PlayerLoginDetail::requestedName)
        .DefMember("tankIdName", &PlayerLoginDetail::tankIDName)
        .DefMember("tankIdPass", &PlayerLoginDetail::tankIDPass)
        .DefMember("gameVersion", &PlayerLoginDetail::gameVersion)
        .DefMember("platformType", &PlayerLoginDetail::platformType)
        .DefMember("country", &PlayerLoginDetail::country)
        .DefMember("protocol", &PlayerLoginDetail::protocol)
        .DefMember("loginMode", &PlayerLoginDetail::loginMode)
        .DefMember("meta", &PlayerLoginDetail::meta)
        .DefMember("mac", &PlayerLoginDetail::mac)
        .DefMember("rid", &PlayerLoginDetail::rid)
        .DefMember("sid", &PlayerLoginDetail::sid)
        .DefMember("gid", &PlayerLoginDetail::gid)
        .DefMember("vid", &PlayerLoginDetail::vid)
        .DefMember("token", &PlayerLoginDetail::token)
        .DefMember("user", &PlayerLoginDetail::user)
        .DefMember("doorId", &PlayerLoginDetail::doorID);

    auto inventory = pLuaManager->BindClass<PlayerInventory>("PlayerInventory");
    inventory.DefMethod("getItemById", &PlayerInventory::GetItemByID)
        .DefMethod("getClothByPart", &PlayerInventory::GetClothByPart)
        .DefMethod("setClothByPart", &PlayerInventory::SetClothByPart)
        .DefMethod("isWearingItem", &PlayerInventory::IsWearingItem)
        .DefMethod("haveRoomForItem", &PlayerInventory::HaveRoomForItem)
        .DefMethod("getFitItemCount", &PlayerInventory::GetFitItemCount)
        .DefMethod("getCountOfItem", &PlayerInventory::GetCountOfItem)
        .DefMethod("updateInventory", &PlayerInventory::UpdateInventory)
        .DefMethod("removeFromQuickSlots", &PlayerInventory::RemoveFromQuickSlots)
        .DefMethod("isWearingPlayMod", &PlayerInventory::IsWearingPlayMod)
        .DefMethod("getInventorySize", &PlayerInventory::GetInventoryCapacity)
        .DefMethod("getItemsSize", &PlayerInventory::GetItemsSize)
        .DefMethod("getInventorySpace", &PlayerInventory::GetInventorySpace);

    auto modController = pLuaManager->BindClass<PlayerPlayModController>("PlayerPlayModController");
    modController.DefMethod("addPlayMod", &PlayerPlayModController::AddPlayMod)
        .DefMethod("removePlayMod", &PlayerPlayModController::RemovePlayMod)
        .DefMethod("hasPlayMod", &PlayerPlayModController::HasPlayMod)
        .DefMethod("getActiveMod", &PlayerPlayModController::GetActiveMod)
        .DefMethod("getActiveModCount", &PlayerPlayModController::GetActiveModCount)
        .DefMethod("recalculateStats", &PlayerPlayModController::RecalculateStats);

    auto achieveInfo = pLuaManager->BindClass<AchievementInfo>("AchievementInfo");
    achieveInfo.DefMember("name", &AchievementInfo::name)
        .DefMember("description", &AchievementInfo::description)
        .DefMember("achievement", &AchievementInfo::achievement)
        .DefMember("achievementType", &AchievementInfo::achievementType)
        .DefMember("progressType", &AchievementInfo::progressType)
        .DefMember("requiredValue", &AchievementInfo::requiredValue);

    auto achieveMgr = pLuaManager->BindClass<AchievementManager>("AchievementManager");
    achieveMgr.DefMethod("getAchievement", &AchievementManager::GetAchievement);

    auto extraData = pLuaManager->BindClass<PlayerExtraData>("PlayerExtraData");
    bool (PlayerExtraData::*setVariant)(ePlayerExtraKey, const Variant&) = &PlayerExtraData::Set;
    extraData.DefMethod("set", setVariant)
        .DefMethod("get", &PlayerExtraData::Get)
        .DefMethod("getMemEstimate", &PlayerExtraData::GetMemEstimate);

    auto progress = pLuaManager->BindClass<PlayerProgress>("PlayerProgress");
    progress.DefMethod("getProgress", &PlayerProgress::GetProgress)
        .DefMethod("addProgress", &PlayerProgress::AddProgress)
        .DefMethod("setProgress", &PlayerProgress::SetProgress)
        .DefMethod("hasAchievement", &PlayerProgress::HasAchievement)
        .DefMethod("getAchievementProgress", &PlayerProgress::GetAchievementProgress)
        .DefMethod("unlockAchievement", &PlayerProgress::UnlockAchievement)
        .DefMethod("checkAchieveAndUnlockIfPossibleByProgress",
                   &PlayerProgress::CheckAchieveAndUnlockIfPossibleByProgress)
        .DefMethod("getCountOfCompletedAchieves", &PlayerProgress::GetCountOfCompletedAchieves)
        .DefMethod("hasTitle", &PlayerProgress::HasTitle)
        .DefMethod("modifyOwnedTitle", &PlayerProgress::ModifyOwnedTitle)
        .DefMethod("isTitleActive", &PlayerProgress::IsTitleActive)
        .DefMethod("modifyTitleActivation", &PlayerProgress::ModifyTitleActivation)
        .DefMethod("getBattlePetName", &PlayerProgress::GetBattlePetName)
        .DefMethod("isBattleLeashFull", &PlayerProgress::IsBattleLeashFull);

    auto playerManager = pLuaManager->BindClass<PlayerManager>("PlayerManager");
    playerManager.DefMethod("getPlayerByNetId", &PlayerManager::GetPlayerByNetID)
        .DefMethod("getPlayerByUserId", &PlayerManager::GetPlayerByUserID)
        .DefMethod("getPlayerCount", &PlayerManager::GetPlayerCount)
        .DefMethod("getTotalPlayerCount", &PlayerManager::GetTotalPlayerCount)
        .DefMethod("saveAllToDatabase", &PlayerManager::SaveAllToDatabase)
        .DefMethodDefault("broadcastMessage", &PlayerManager::BroadcastMessage, string(""), string(""));

    pLuaManager->DefFunction("getAchievementManager", &GetAchievementManager);
    pLuaManager->DefFunction("getPlayerManager", &GetPlayerManager);
}

#endif