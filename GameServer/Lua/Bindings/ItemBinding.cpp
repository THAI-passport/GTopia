#ifdef ENABLE_LUA_SCRIPTING

#include "ItemBinding.h"
#include "../LuaManager.h"
#include "Item/ItemInfo.h"
#include "Item/ItemInfoManager.h"

void RegisterItemBinding()
{
    LuaManager* pLuaManager = GetLuaManager();

    auto itemChairInfo = pLuaManager->BindClass<ItemChairInfo>("ItemChairInfo");
    itemChairInfo.DefMember("enabled", &ItemChairInfo::enabled)
        .DefMember("playerOffset", &ItemChairInfo::playerOffset)
        .DefMember("armPos", &ItemChairInfo::armPos)
        .DefMember("armOffset", &ItemChairInfo::armOffset)
        .DefMember("armTexture", &ItemChairInfo::armTexture);

    auto itemRandomSpriteInfo = pLuaManager->BindClass<ItemRandomSpriteReplaceInfo>("ItemRandomSpriteReplaceInfo");
    itemRandomSpriteInfo.DefMember("enabled", &ItemRandomSpriteReplaceInfo::enabled)
        .DefMember("offsetMod", &ItemRandomSpriteReplaceInfo::offsetMod)
        .DefMember("chance", &ItemRandomSpriteReplaceInfo::chance);

    auto consumableInfo = pLuaManager->BindClass<ConsumableInfo>("ConsumableInfo");
    consumableInfo.DefMember("itemId", &ConsumableInfo::itemID)
        .DefMember("consumableType", &ConsumableInfo::consumableType)
        .DefMember("requiredAmount", &ConsumableInfo::requiredAmount)
        .DefMember("rewardItemId", &ConsumableInfo::rewardItemID)
        .DefMember("rewardCount", &ConsumableInfo::rewardCount)
        .DefMember("flags", &ConsumableInfo::flags)
        .DefMember("successMessage", &ConsumableInfo::successMessage)
        .DefMember("failMessage", &ConsumableInfo::failMessage)
        .DefMethod("hasFlag", &ConsumableInfo::HasFlag);

    auto itemInfo = pLuaManager->BindClass<ItemInfo>("ItemInfo");
    itemInfo.DefMember("id", &ItemInfo::id)
        .DefMember("flags", &ItemInfo::flags)
        .DefMember("type", &ItemInfo::type)
        .DefMember("material", &ItemInfo::material)
        .DefMember("name", &ItemInfo::name)
        .DefMember("textureFile", &ItemInfo::textureFile)
        .DefMember("textureHash", &ItemInfo::textureHash)
        .DefMember("visualEffect", &ItemInfo::visualEffect)
        .DefMember("cookingTime", &ItemInfo::cookingTime)
        .DefMember("textureX", &ItemInfo::textureX)
        .DefMember("textureY", &ItemInfo::textureY)
        .DefMember("storage", &ItemInfo::storage)
        .DefMember("layer", &ItemInfo::layer)
        .DefMember("collisionType", &ItemInfo::collisionType)
        .DefMember("hp", &ItemInfo::hp)
        .DefMember("restoreTime", &ItemInfo::restoreTime)
        .DefMember("bodyPart", &ItemInfo::bodyPart)
        .DefMember("rarity", &ItemInfo::rarity)
        .DefMember("maxCanHold", &ItemInfo::maxCanHold)
        .DefMember("extraString", &ItemInfo::extraString)
        .DefMember("extraStringHash", &ItemInfo::extraStringHash)
        .DefMember("animMs", &ItemInfo::animMS)
        .DefMember("petName", &ItemInfo::petName)
        .DefMember("petSubName", &ItemInfo::petSubName)
        .DefMember("petEndName", &ItemInfo::petEndName)
        .DefMember("petPowerName", &ItemInfo::petPowerName)
        .DefMember("seedBg", &ItemInfo::seedBg)
        .DefMember("seedFg", &ItemInfo::seedFg)
        .DefMember("treeBg", &ItemInfo::treeBg)
        .DefMember("treeFg", &ItemInfo::treeFg)
        .DefMember("seed1", &ItemInfo::seed1)
        .DefMember("seed2", &ItemInfo::seed2)
        .DefMember("growTime", &ItemInfo::growTime)
        .DefMember("fxFlags", &ItemInfo::fxFlags)
        .DefMember("multiAnim1", &ItemInfo::multiAnim1)
        .DefMember("overlayTextureFile", &ItemInfo::overlayTextureFile)
        .DefMember("multiAnim2", &ItemInfo::multiAnim2)
        .DefMember("dualAnimLayer", &ItemInfo::dualAnimLayer)
        .DefMember("flags2", &ItemInfo::flags2)
        .DefMember("tileRange", &ItemInfo::tileRange)
        .DefMember("pileSize", &ItemInfo::pileSize)
        .DefMember("customizedPunchParameters", &ItemInfo::customizedPunchParameters)
        .DefMember("lightSourceRange", &ItemInfo::lightSourceRange)
        .DefMember("variantVersionItem", &ItemInfo::variantVersionItem)
        .DefMember("chairInfo", &ItemInfo::chairInfo)
        .DefMember("configName", &ItemInfo::configName)
        .DefMember("otherPlayerHitParticle", &ItemInfo::otherPlayerHitParticle)
        .DefMember("randomSpriteInfo", &ItemInfo::randomSpriteInfo)
        .DefMember("canTransform", &ItemInfo::canTransform)
        .DefMember("slipperyType", &ItemInfo::slipperyType)
        .DefMember("description", &ItemInfo::description)
        .DefMember("element", &ItemInfo::element)
        .DefMember("maxFruitCount", &ItemInfo::maxFruitCount)
        .DefMember("farmability", &ItemInfo::farmablity)
        .DefMethod("hasFlag", &ItemInfo::HasFlag)
        .DefMethod("hasFlag2", &ItemInfo::HasFlag2)
        .DefMethod("isUnlimited", &ItemInfo::IsUnlimited)
        .DefMethod("isBackground", &ItemInfo::IsBackground)
        .DefMethod("isLosBlocking", &ItemInfo::IsLOSBlocking);

    auto itemInfoMgr = pLuaManager->BindClass<ItemInfoManager>("ItemInfoManager");
    itemInfoMgr.DefMethod("getItemById", &ItemInfoManager::GetItemByID)
        .DefMethod("getItemByName", &ItemInfoManager::GetItemByName)
        .DefMethod("getItemCount", &ItemInfoManager::GetItemCount)
        .DefMethod("getSpliceInfo", &ItemInfoManager::GetSpliceInfo)
        .DefMethod("getConsumableInfo", &ItemInfoManager::GetConsumableInfo)
        .DefMethod("getBattlePetInfo", &ItemInfoManager::GetBattlePetInfo)
        .DefMethod("getSortedItemIdsByName", &ItemInfoManager::GetSortedItemIdsByName);

    pLuaManager->DefFunction("getItemInfoManager", &GetItemInfoManager);

    pLuaManager->DefFunction("isIllegalItem", &IsIllegalItem);
    pLuaManager->DefFunction("isWorldLock", &IsWorldLock);
    pLuaManager->DefFunction("isMainDoor", &IsMainDoor);
    pLuaManager->DefFunction("isFuelPack", &IsFuelPack);
    pLuaManager->DefFunction("isJammer", &IsJammer);
    pLuaManager->DefFunction("isGauntletOfElements", &IsGaunletOfElements);
    pLuaManager->DefFunction("isPathMarker", &IsPathMarker);
    pLuaManager->DefFunction("getMaxTilesToLock", &GetMaxTilesToLock);
    pLuaManager->DefFunction("getBaseItemId", &GetBaseItemID);
    pLuaManager->DefFunction("toItemClientId", &ToItemClientID);
    pLuaManager->DefFunction("seedToItemId", &SEED_TO_ITEM_ID);
    pLuaManager->DefFunction("itemToSeedId", &ITEM_TO_SEED_ID);
}

#endif