#ifdef ENABLE_LUA_SCRIPTING

#include "../../World/World.h"
#include "../../World/WorldManager.h"
#include "../LuaManager.h"
#include "PlayerBinding.h"

void RegisterWorldBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    auto worldObj = pLuaManager->BindClass<WorldObject>("WorldObject");
    worldObj.DefMember("itemId", &WorldObject::itemID)
        .DefMember("pos", &WorldObject::pos)
        .DefMember("count", &WorldObject::count)
        .DefMember("flags", &WorldObject::flags)
        .DefMember("objectId", &WorldObject::objectID)
        .DefMethod("getCenterPos", &WorldObject::GetCenterPos)
        .DefMethod("hasFlag", &WorldObject::HasFlag);

    auto worldObjMgr = pLuaManager->BindClass<WorldObjectManager>("WorldObjectManager");
    worldObjMgr.DefMethod("getMemEstimate", &WorldObjectManager::GetMemEstimate)
        .DefMethod("addItem", &WorldObjectManager::AddItem)
        .DefMethod("modifyItem", &WorldObjectManager::ModifyItem)
        .DefMethod("deleteById", &WorldObjectManager::DeleteByID)
        .DefMethod("getObjectById", &WorldObjectManager::GetObjectByID)
        .DefMethod("getCountOfObjectsInRect", &WorldObjectManager::GetCounfOfObjestsInRect);

    auto tileMailboxLetter = pLuaManager->BindClass<TileMailboxLetter>("TileMailboxLetter");
    tileMailboxLetter.DefMember("userId", &TileMailboxLetter::userID).DefMember("message", &TileMailboxLetter::message);

    auto tileDonationBoxGifts = pLuaManager->BindClass<TileDonationBoxGifts>("TileDonationBoxGifts");
    tileDonationBoxGifts.DefMember("userId", &TileDonationBoxGifts::userID)
        .DefMember("message", &TileDonationBoxGifts::message)
        .DefMember("itemId", &TileDonationBoxGifts::itemID)
        .DefMember("itemCount", &TileDonationBoxGifts::itemCount);

    auto tileExtra = pLuaManager->BindClass<TileExtra>("TileExtra");
    tileExtra.DefMember("type", &TileExtra::type)
        .DefMethod("getGrowthPercent", &TileExtra::GetGrowthPercent)
        .DefMethod("finalizeGrowth", &TileExtra::FinalizeGrowth)
        .DefMethod("modGrowth", &TileExtra::ModGrowth);

    auto tileExtraGrowth = pLuaManager->BindClass<TileExtraGrowth>("TileExtraGrowth");
    tileExtraGrowth.DefMember("timer", &TileExtraGrowth::timer).DefMember("growTime", &TileExtraGrowth::growTime);

    auto tileExtraDoor = pLuaManager->BindClass<TileExtra_Door>("TileExtra_Door");
    tileExtraDoor.DefMember("name", &TileExtra_Door::name)
        .DefMember("text", &TileExtra_Door::text)
        .DefMember("id", &TileExtra_Door::id)
        .DefMember("flags", &TileExtra_Door::flags)
        .DefMethod("setFlag", &TileExtra_Door::SetFlag)
        .DefMethod("removeFlag", &TileExtra_Door::RemoveFlag)
        .DefMethod("hasFlag", &TileExtra_Door::HasFlag);

    auto tileExtraSign = pLuaManager->BindClass<TileExtra_Sign>("TileExtra_Sign");
    tileExtraSign.DefMember("text", &TileExtra_Sign::text);

    auto tileExtraLock = pLuaManager->BindClass<TileExtra_Lock>("TileExtra_Lock");
    tileExtraLock.DefMember("flags", &TileExtra_Lock::flags)
        .DefMember("ownerId", &TileExtra_Lock::ownerID)
        .DefMember("accessList", &TileExtra_Lock::accessList)
        .DefMember("minEntryLevel", &TileExtra_Lock::minEntryLevel)
        .DefMember("worldTimer", &TileExtra_Lock::worldTimer)
        .DefMethod("setFlag", &TileExtra_Lock::SetFlag)
        .DefMethod("removeFlag", &TileExtra_Lock::RemoveFlag)
        .DefMethod("hasFlag", &TileExtra_Lock::HasFlag)
        .DefMethod("getTempo", &TileExtra_Lock::GetTempo)
        .DefMethod("setTempo", &TileExtra_Lock::SetTempo)
        .DefMethod("hasAccess", &TileExtra_Lock::HasAccess)
        .DefMethod("isAdmin", &TileExtra_Lock::IsAdmin)
        .DefMethod("getTotalAccessedCount", &TileExtra_Lock::GetTotalAccessedCount)
        .DefMethod("removeFromList", &TileExtra_Lock::RemoveFromList);

    auto tileExtraSeed = pLuaManager->BindClass<TileExtra_Seed>("TileExtra_Seed");
    tileExtraSeed.DefMember("fruitCount", &TileExtra_Seed::fruitCount);

    auto tileExtraComponent = pLuaManager->BindClass<TileExtra_Component>("TileExtra_Component");
    tileExtraComponent.DefMember("randValue", &TileExtra_Component::randValue);

    auto tileExtraProvider = pLuaManager->BindClass<TileExtra_Provider>("TileExtra_Provider");
    tileExtraProvider.DefMember("otherData", &TileExtra_Provider::otherData);

    auto tileExtraAchievement = pLuaManager->BindClass<TileExtra_Achievement>("TileExtra_Achievement");
    tileExtraAchievement.DefMember("ownerId", &TileExtra_Achievement::ownerID)
        .DefMember("achievementId", &TileExtra_Achievement::achievementID);

    auto tileExtraHeartMonitor = pLuaManager->BindClass<TileExtra_HeartMonitor>("TileExtra_HeartMonitor");
    tileExtraHeartMonitor.DefMember("ownerId", &TileExtra_HeartMonitor::ownerID)
        .DefMember("playerDisplayName", &TileExtra_HeartMonitor::playerDisplayName);

    auto tileExtraXenonite = pLuaManager->BindClass<TileExtra_Xenonite>("TileExtra_Xenonite");
    tileExtraXenonite.DefMember("flags", &TileExtra_Xenonite::flags)
        .DefMember("flags2", &TileExtra_Xenonite::flags2)
        .DefMethod("setFlag", &TileExtra_Xenonite::SetFlag)
        .DefMethod("removeFlag", &TileExtra_Xenonite::RemoveFlag)
        .DefMethod("hasFlag", &TileExtra_Xenonite::HasFlag)
        .DefMethod("setFlag2", &TileExtra_Xenonite::SetFlag2)
        .DefMethod("removeFlag2", &TileExtra_Xenonite::RemoveFlag2)
        .DefMethod("hasFlag2", &TileExtra_Xenonite::HasFlag2);

    auto tileExtraOuijaBoard = pLuaManager->BindClass<TileExtra_OuijaBoard>("TileExtra_OuijaBoard");
    tileExtraOuijaBoard.DefMember("playerCount", &TileExtra_OuijaBoard::playerCount)
        .DefMember("ouijaType", &TileExtra_OuijaBoard::ouijaType)
        .DefMember("command", &TileExtra_OuijaBoard::command)
        .DefMember("items", &TileExtra_OuijaBoard::items);

    auto tileExtraFieldNode = pLuaManager->BindClass<TileExtra_FieldNode>("TileExtra_FieldNode");
    tileExtraFieldNode.DefMember("expireTime", &TileExtra_FieldNode::expireTime)
        .DefMember("nodes", &TileExtra_FieldNode::nodes);

    auto tileExtraBattleCage = pLuaManager->BindClass<TileExtra_BattleCage>("TileExtra_BattleCage");
    tileExtraBattleCage.DefMember("cageName", &TileExtra_BattleCage::cageName)
        .DefMember("basePet", &TileExtra_BattleCage::basePet)
        .DefMember("secondPet", &TileExtra_BattleCage::secondPet)
        .DefMember("thirdPet", &TileExtra_BattleCage::thirdPet);

    auto tileExtraPetTrainer = pLuaManager->BindClass<TileExtra_PetTrainer>("TileExtra_PetTrainer");
    tileExtraPetTrainer.DefMember("trainerName", &TileExtra_PetTrainer::trainerName)
        .DefMember("pets", &TileExtra_PetTrainer::pets);

    auto tileExtraMailbox = pLuaManager->BindClass<TileExtra_Mailbox>("TileExtra_Mailbox");
    tileExtraMailbox.DefMember("letters", &TileExtra_Mailbox::letters)
        .DefMethod("hasLetterFromId", &TileExtra_Mailbox::HasLetterFromID);

    auto tileExtraCrystal = pLuaManager->BindClass<TileExtra_Crystal>("TileExtra_Crystal");
    tileExtraCrystal.DefMember("crystals", &TileExtra_Crystal::crystals);

    auto tileExtraBulletin = pLuaManager->BindClass<TileExtra_Bulletin>("TileExtra_Bulletin");
    tileExtraBulletin.DefMember("letters", &TileExtra_Bulletin::letters)
        .DefMember("flags", &TileExtra_Bulletin::flags)
        .DefMethod("setFlag", &TileExtra_Bulletin::SetFlag)
        .DefMethod("removeFlag", &TileExtra_Bulletin::RemoveFlag)
        .DefMethod("hasFlag", &TileExtra_Bulletin::HasFlag)
        .DefMethod("getCountOfLettersFromId", &TileExtra_Bulletin::GetCountOfLettersFromID);

    auto tileExtraDonationBox = pLuaManager->BindClass<TileExtra_DonaitonBox>("TileExtra_DonationBox");
    tileExtraDonationBox.DefMember("gifts", &TileExtra_DonaitonBox::gifts)
        .DefMember("minRarity", &TileExtra_DonaitonBox::minRarity)
        .DefMethod("getCountOfGiftsFromId", &TileExtra_DonaitonBox::GetCountOfGiftsFromID);

    auto tileExtraWeatherSpecial = pLuaManager->BindClass<TileExtra_WeatherSpecial>("TileExtra_WeatherSpecial");
    tileExtraWeatherSpecial.DefMember("itemId", &TileExtra_WeatherSpecial::itemID)
        .DefMember("color", &TileExtra_WeatherSpecial::color)
        .DefMember("gravity", &TileExtra_WeatherSpecial::gravity)
        .DefMember("cycleTime", &TileExtra_WeatherSpecial::cycleTime)
        .DefMember("flags", &TileExtra_WeatherSpecial::flags)
        .DefMethod("setFlag", &TileExtra_WeatherSpecial::SetFlag)
        .DefMethod("removeFlag", &TileExtra_WeatherSpecial::RemoveFlag)
        .DefMethod("hasFlag", &TileExtra_WeatherSpecial::HasFlag);

    auto tileExtraMannequin = pLuaManager->BindClass<TileExtra_Mannequin>("TileExtra_Mannequin");
    tileExtraMannequin.DefMember("text", &TileExtra_Mannequin::text)
        .DefMember("flags", &TileExtra_Mannequin::flags)
        .DefMember("hairColor", &TileExtra_Mannequin::hairColor)
        .DefMember("charFlags", &TileExtra_Mannequin::charFlags)
        .DefMember("char2Flags", &TileExtra_Mannequin::char2Flags)
        .DefMethod("setCloth", &TileExtra_Mannequin::SetCloth);

    auto tileExtraMagicEgg = pLuaManager->BindClass<TileExtra_MagicEgg>("TileExtra_MagicEgg");
    tileExtraMagicEgg.DefMember("eggCount", &TileExtra_MagicEgg::eggCount);

    auto tileExtraDressup = pLuaManager->BindClass<TileExtra_Dressup>("TileExtra_Dressup");
    tileExtraDressup.DefMethod("setCloth", &TileExtra_Dressup::SetCloth);

    auto tileExtraSpotlight = pLuaManager->BindClass<TileExtra_Spotlight>("TileExtra_Spotlight");
    tileExtraSpotlight.DefMember("playerNetId", &TileExtra_Spotlight::playerNetID);

    auto tileExtraDisplayBlock = pLuaManager->BindClass<TileExtra_DisplayBlock>("TileExtra_DisplayBlock");
    tileExtraDisplayBlock.DefMember("itemId", &TileExtra_DisplayBlock::itemID);

    auto tileExtraVending = pLuaManager->BindClass<TileExtra_VendingMachine>("TileExtra_VendingMachine");
    tileExtraVending.DefMember("itemId", &TileExtra_VendingMachine::itemID)
        .DefMember("price", &TileExtra_VendingMachine::price)
        .DefMember("stock", &TileExtra_VendingMachine::stock)
        .DefMember("worldLockCount", &TileExtra_VendingMachine::worldLockCount);

    auto tileExtraSucker = pLuaManager->BindClass<TileExtra_Sucker>("TileExtra_Sucker");
    tileExtraSucker.DefMember("itemId", &TileExtra_Sucker::itemID)
        .DefMember("count", &TileExtra_Sucker::count)
        .DefMember("isSucking", &TileExtra_Sucker::isSucking)
        .DefMember("isPlanting", &TileExtra_Sucker::isPlanting);

    auto tileInfo = pLuaManager->BindClass<TileInfo>("TileInfo");
    tileInfo.DefMethod("getFg", &TileInfo::GetFG)
        .DefMethod("getBg", &TileInfo::GetBG)
        .DefMethod("isCollidable", &TileInfo::IsCollidable)
        .DefMethod("getPos", &TileInfo::GetPos)
        .DefMethod("getMapIndex", &TileInfo::GetMapIndex)
        .DefMethod("getWorldPos", &TileInfo::GetWorldPos)
        .DefMethod("getWorldPosCenter", &TileInfo::GetWorldPosCenter)
        .DefMethod("getRect", &TileInfo::GetRect)
        .DefMethod("setFlag", &TileInfo::SetFlag)
        .DefMethod("removeFlag", &TileInfo::RemoveFlag)
        .DefMethod("hasFlag", &TileInfo::HasFlag)
        .DefMethod("toggleFlag", &TileInfo::ToggleFlag)
        .DefMethod("punchTile", &TileInfo::PunchTile)
        .DefMethod("getHealthPercent", &TileInfo::GetHealthPercent)
        .DefMethod("isCrystal", &TileInfo::IsCrystal)
        .DefMethod("isTree", &TileInfo::IsTree)
        .DefMethod("isFlammable", &TileInfo::IsFlammable)
        .DefMethod("getGrowthPercent", &TileInfo::GetGrowthPercent)
        .DefMethod("getTileExtraType", &TileInfo::GetTileExtraType)
        .DefMethod("hasExtra", &TileInfo::HasExtra)
        .DefMethod("getExtraDoor", &TileInfo::GetExtra<TileExtra_Door>)
        .DefMethod("getExtraSign", &TileInfo::GetExtra<TileExtra_Sign>)
        .DefMethod("getExtraLock", &TileInfo::GetExtra<TileExtra_Lock>)
        .DefMethod("getExtraSeed", &TileInfo::GetExtra<TileExtra_Seed>)
        .DefMethod("getExtraComponent", &TileInfo::GetExtra<TileExtra_Component>)
        .DefMethod("getExtraProvider", &TileInfo::GetExtra<TileExtra_Provider>)
        .DefMethod("getExtraAchievement", &TileInfo::GetExtra<TileExtra_Achievement>)
        .DefMethod("getExtraHeartMonitor", &TileInfo::GetExtra<TileExtra_HeartMonitor>)
        .DefMethod("getExtraXenonite", &TileInfo::GetExtra<TileExtra_Xenonite>)
        .DefMethod("getExtraOuijaBoard", &TileInfo::GetExtra<TileExtra_OuijaBoard>)
        .DefMethod("getExtraFieldNode", &TileInfo::GetExtra<TileExtra_FieldNode>)
        .DefMethod("getExtraBattleCage", &TileInfo::GetExtra<TileExtra_BattleCage>)
        .DefMethod("getExtraPetTrainer", &TileInfo::GetExtra<TileExtra_PetTrainer>)
        .DefMethod("getExtraMailbox", &TileInfo::GetExtra<TileExtra_Mailbox>)
        .DefMethod("getExtraCrystal", &TileInfo::GetExtra<TileExtra_Crystal>)
        .DefMethod("getExtraBulletin", &TileInfo::GetExtra<TileExtra_Bulletin>)
        .DefMethod("getExtraDonationBox", &TileInfo::GetExtra<TileExtra_DonaitonBox>)
        .DefMethod("getExtraWeatherSpecial", &TileInfo::GetExtra<TileExtra_WeatherSpecial>)
        .DefMethod("getExtraMannequin", &TileInfo::GetExtra<TileExtra_Mannequin>)
        .DefMethod("getExtraMagicEgg", &TileInfo::GetExtra<TileExtra_MagicEgg>)
        .DefMethod("getExtraDressup", &TileInfo::GetExtra<TileExtra_Dressup>)
        .DefMethod("getExtraSpotlight", &TileInfo::GetExtra<TileExtra_Spotlight>)
        .DefMethod("getExtraDisplayBlock", &TileInfo::GetExtra<TileExtra_DisplayBlock>)
        .DefMethod("getExtraVendingMachine", &TileInfo::GetExtra<TileExtra_VendingMachine>)
        .DefMethod("getExtraSucker", &TileInfo::GetExtra<TileExtra_Sucker>);

    auto worldTileMgr = pLuaManager->BindClass<WorldTileManager>("WorldTileManager");
    TileInfo* (WorldTileManager::*getTileXY)(int32, int32) = &WorldTileManager::GetTile;
    TileInfo* (WorldTileManager::*getTilePos)(const Vector2Int&) = &WorldTileManager::GetTile;
    TileInfo* (WorldTileManager::*getTileWorldPos)(const Vector2Float&) = &WorldTileManager::GetTileByWorldPos;
    worldTileMgr.DefMethod("getSize", &WorldTileManager::GetSize)
        .DefMethod("getTile", getTileXY)
        .DefMethod("getTileByPos", getTilePos)
        .DefMethod("getKeyTile", &WorldTileManager::GetKeyTile)
        .DefMethod("getTileByWorldPos", getTileWorldPos)
        .DefMethod("isTileLockedWithLock", &WorldTileManager::IsTileLockedWithLock)
        .DefMethod("isTileLockedWithLockButPublic", &WorldTileManager::IsTileLockedWithLockButPublic)
        .DefMethod("isPlayerOwnerOfTheTile", &WorldTileManager::IsPlayerOwnerOfTheTile)
        .DefMethod("getTileIndex", &WorldTileManager::GetTileIndex)
        .DefMethod("canPlantTreeHere", &WorldTileManager::CanPlantTreeHere)
        .DefMethod("getLockCount", &WorldTileManager::GetLockCount);

    auto worldNpc = pLuaManager->BindClass<WorldNPC>("WorldNPC");
    worldNpc.DefMember("id", &WorldNPC::id)
        .DefMember("type", &WorldNPC::type)
        .DefMember("pos", &WorldNPC::pos)
        .DefMember("dest", &WorldNPC::dest)
        .DefMember("speed", &WorldNPC::speed)
        .DefMember("val1", &WorldNPC::val1)
        .DefMember("val2", &WorldNPC::val2)
        .DefMember("hp", &WorldNPC::hp)
        .DefMethod("moveGhostRandom", &WorldNPC::MoveGhostRandom)
        .DefMethod("checkGhostCanSlime", &WorldNPC::CheckGhostCanSlime)
        .DefMethod("isGhost", &WorldNPC::IsGhost)
        .DefMethod("isGhostTrap", &WorldNPC::IsGhostTrap)
        .DefMethod("isInside", &WorldNPC::IsInside);

    auto worldNpcMgr = pLuaManager->BindClass<WorldNPCManager>("WorldNPCManager");
    worldNpcMgr.DefMethod("spawnGhost", &WorldNPCManager::SpawnGhost)
        .DefMethod("spawnGhostTrap", &WorldNPCManager::SpawnGhostTrap)
        .DefMethod("removeNpc", &WorldNPCManager::RemoveNpc)
        .DefMethod("getNpcById", &WorldNPCManager::GetNPCByID)
        .DefMethod("getNpcActiveOrNotCount", &WorldNPCManager::GetNPCActiveOrNotCount)
        .DefMethod("getLastAddedNpcId", &WorldNPCManager::GetLastAddedNPCID);

    auto worldMgr = pLuaManager->BindClass<WorldManager>("WorldManager");
    worldMgr.DefMethod("getWorldByInstanceId", &WorldManager::GetWorldByInstanceID)
        .DefMethod("getWorldByName", &WorldManager::GetWorldByName)
        .DefMethod("getWorldByDatabaseId", &WorldManager::GetWorldByDatabaseID)
        .DefMethod("saveAllToDatabase", &WorldManager::SaveAllToDatabase)
        .DefMethod("sendWorldMenuRequest", &WorldManager::SendWorldMenuRequest)
        .DefMethod("getWorldCount", &WorldManager::GetWorldCount);

    pLuaManager->DefFunction("getWorldManager", &GetWorldManager);

    auto world = pLuaManager->BindClass<World>("World");
    void (World::*sendParticleXY)(float, float, uint32, float, int32) = &World::SendParticleEffectToAll;
    void (World::*sendTileUpdate)(TileInfo*) = &World::SendTileUpdate;
    world.DefMethod("getDatabaseId", &World::GetDatabaseID)
        .DefMethod("getState", &World::GetState)
        .DefMethod("getInstanceId", &World::GetInstanceID)
        .DefMethod("getPlayerCount", &World::GetPlayerCount)
        .DefMethod("getNpcManager", &World::GetNPCManager)
        .DefMethod("sendTalkBubbleAndConsoleToAll", &World::SendTalkBubbleAndConsoleToAll)
        .DefMethod("sendConsoleMessageToAll", &World::SendConsoleMessageToAll)
        .DefMethod("sendParticleEffectToAll", sendParticleXY)
        .DefMethod("sendTileUpdate", sendTileUpdate)
        .DefMethod("playSfxForEveryone", &World::PlaySFXForEveryone)
        .DefMethod("sendOnAddNotificationToAll", &World::SendOnAddNotificationToAll)
        .DefMethod("throwItemToPlayerFromPosition", &World::ThrowItemToPlayerFromPosition)
        .DefMethod("throwItemToPositionFromPlayer", &World::ThrowItemToPositionFromPlayer)
        .DefMethod("dropGemsOnTile", &World::DropGemsOnTile)
        .DefMethod("dropObjectOnTile", &World::DropObjectOnTile)
        .DefMethod("flameUpTile", &World::FlameUpTile)
        .DefMethod("putOutFire", &World::PutOutFire)
        .DefMethod("isPlayerWorldOwner", &World::IsPlayerWorldOwner)
        .DefMethod("isPlayerWorldAdmin", &World::IsPlayerWorldAdmin)
        .DefMethod("getWorldOwnerId", &World::GetWorldOwnerID)
        .DefMethod("getPlayerByNameStartsWith", &World::GetPlayerByNameStartsWith)

        // WorldInfo
        .DefMethod("setName", &WorldInfo::SetName)
        .DefMethod("getWorldName", &WorldInfo::GetWorldName)
        .DefMethod("setCurrentWeather", &WorldInfo::SetCurrentWeather)
        .DefMethod("getCurrentWeather", &WorldInfo::GetCurrentWeather)
        .DefMethod("getDefaultWeather", &WorldInfo::GetDefaultWeather)
        .DefMethod("getWorldVersion", &WorldInfo::GetWorldVersion)
        .DefMethod("getTileManager", &WorldInfo::GetTileManager)
        .DefMethod("getObjectManager", &WorldInfo::GetObjectManager);
}

#endif