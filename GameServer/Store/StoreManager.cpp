#include "StoreManager.h"
#include "Holiday/HolidayManager.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Utils/ConfigDB.h"

StoreManager::StoreManager() {}

StoreManager::~StoreManager() {}

int32 StoreEntry::GetCost(GamePlayer* pPlayer)
{
    int32 finalCost = cost;

    if (!pPlayer)
        return finalCost;

    if (code == "upgrade_backpack")
    {
        // initial inventory size is 16 and giving 10 spaces every purchase
        int32 upgradeCount = (pPlayer->GetInventory().GetInventoryCapacity() - 16) / 10;
        finalCost = (upgradeCount * upgradeCount * 100) + 100;
    }

    return finalCost;
}

bool StoreManager::Load(const string& filePath)
{
    m_storeEntries->clear();
    m_storeSearch.clear();

    ConfigDB cfg;
    if (!cfg.Load(filePath))
        return false;

    for (auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if (key == "add_store")
        {
            if (!line.Require(8))
                return false;

            int32 storeTab = line.GetInt(1, STORE_TAB_COUNT);
            if (storeTab >= STORE_TAB_COUNT)
            {
                LOGGER_LOG_ERROR("Unknown store tab id %d, line %d", storeTab, line.lineNumber);
                return false;
            }

            StoreEntry entry;
            entry.code = line.GetString(2);
            entry.name = line.GetString(3);
            entry.cost = line.GetInt(4);
            entry.banner = "interface/large/store_buttons/" + line.GetString(5);
            entry.posX = line.GetUInt(6);
            entry.posY = line.GetUInt(7);
            entry.description = line.GetString(8);
            entry.parentTab = (eStoreTab)storeTab;

            m_storeEntries[storeTab].push_back(std::move(entry));
            continue;
        }

        if (key == "add_tab")
        {
            if (!line.Require(5))
                return false;

            int32 storeTab = line.GetInt(1);
            if (storeTab >= STORE_TAB_COUNT)
            {
                LOGGER_LOG_ERROR("Unknown store tab id %d, line %d", storeTab, line.lineNumber);
                return false;
            }

            StoreEntry entry;
            entry.code = line.GetString(2);
            entry.name = line.GetString(3);
            entry.banner = "interface/large/" + line.GetString(4);
            entry.posY = line.GetUInt(5);
            entry.description = line.GetString(6);
            entry.isTab = true;

            m_storeEntries[storeTab].push_back(std::move(entry));
            continue;
        }

        if (key == "set_items")
        {
            if (!line.Require(2))
                return false;

            uint32 totalArgs = line.GetArgSize() - (line.IsLastSpace() ? 1 : 0);
            if ((totalArgs - 2) % 2 != 0)
            {
                LOGGER_LOG_ERROR("Something wrong with expected args size in line %d", line.lineNumber);
                return false;
            }

            StoreEntry* pEntry = GetEntryByCodeRaw(line.GetString(1));
            if (!pEntry)
                continue;

            for (uint16 i = 2; i < totalArgs; ++i)
            {
                pEntry->items.push_back(line.GetInt(i));
            }

            continue;
        }

        if (key == "set_rule")
        {
            if (!line.Require(1))
                return false;

            StoreEntry* pEntry = GetEntryByCodeRaw(line.GetString(1));
            if (!pEntry)
                continue;

            for (uint16 i = 2; i < line.GetArgSize(); ++i)
            {
                const string& ruleKey = line.GetString(i);

                if (ruleKey == "DISABLED")
                {
                    pEntry->rule.disabled = true;
                    continue;
                }

                if (ruleKey == "CHOOSERANDOM")
                {
                    if (!line.Has(i + 1))
                    {
                        LOGGER_LOG_ERROR("Error on CHOOSERANDOM line %d", line.lineNumber);
                        return false;
                    }

                    pEntry->rule.randomCount = line.GetUInt(i + 1);

                    i++;
                    continue;
                }

                if (ruleKey == "STARTTIME")
                {
                    if (!line.Has(i + 1))
                    {
                        LOGGER_LOG_ERROR("Error on STARTTIME line %d", line.lineNumber);
                        return false;
                    }

                    pEntry->rule.startTime = line.GetUInt(i + 1);

                    i++;
                    continue;
                }

                if (ruleKey == "ENDTIME")
                {
                    if (!line.Has(i + 1))
                    {
                        LOGGER_LOG_ERROR("Error on STAENDTIMERTTIME line %d", line.lineNumber);
                        return false;
                    }

                    pEntry->rule.endTime = line.GetUInt(i + 1);

                    i++;
                    continue;
                }

                if (ruleKey == "HOLIDAY")
                {
                    if (!line.Has(i + 1))
                    {
                        LOGGER_LOG_ERROR("Error on HOLIDAY line %d", line.lineNumber);
                        return false;
                    }

                    pEntry->holidayKey = line.GetString(i + 1);

                    i++;
                    continue;
                }
            }

            continue;
        }
    }

    HashEntries();
    return true;
}

eStoreTab StoreManager::GetTabTypeByCode(const string& entryCode)
{
    for (uint8 i = 0; i < STORE_TAB_COUNT; ++i)
    {
        if (m_storeEntries[i].empty())
            continue;

        StoreEntry& entry = m_storeEntries[i][0];
        if (entry.code == entryCode)
            return (eStoreTab)i;
    }

    return STORE_TAB_NONE;
}

void StoreManager::NavigatePlayer(GamePlayer* pPlayer, eStoreTab storeTab)
{
    if (!pPlayer || storeTab >= STORE_TAB_COUNT)
        return;

    DialogBuilder db;
    BuildStore(pPlayer, db, storeTab);

    pPlayer->SendOnStoreRequest(db.Get());
}

bool StoreManager::PurchaseItem(GamePlayer* pPlayer, const string& entryCode)
{
    if (!pPlayer)
        return false;

    StoreEntry* pStoreEntry = GetStoreEntryByCode(entryCode);
    if (!pStoreEntry)
    {
        LOGGER_LOG_WARN("%s tried to buy non exists %s from store", pPlayer->GetRawName().c_str(), entryCode.c_str());
        return false;
    }

    bool isUpgradeBackpack = (pStoreEntry->code == "upgrade_backpack");

    uint32 epochNow = Time::GetTimeSinceEpoch();
    if (pStoreEntry->rule.disabled || pStoreEntry->rule.startTime > 0 && epochNow < pStoreEntry->rule.startTime ||
        pStoreEntry->rule.endTime > 0 && epochNow > pStoreEntry->rule.endTime ||
        isUpgradeBackpack && pPlayer->GetInventory().GetInventoryCapacity() >= 396 ||
        (!pStoreEntry->holidayKey.empty() && !GetHolidayManager()->IsHolidayActive(pStoreEntry->holidayKey)))
    {
        LOGGER_LOG_WARN("%s tried to buy %s but breaking the rule?!", pPlayer->GetRawName().c_str(), entryCode.c_str());
        return false;
    }

    int32 cost = pStoreEntry->GetCost(pPlayer);

    if (cost == 0)
    {
        pPlayer->SendOnStorePurchaseResult("Something went wrong while calculating the cost.");
        return false;
    }
    else if (cost < 0)
    {
        uint8 tokenCount = pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GROWTOKEN);
        if ((-cost) > tokenCount)
        {
            pPlayer->SendOnStorePurchaseResult("You can't afford " + pStoreEntry->name + "! You're `$" +
                                               ToString((-cost) - tokenCount) + "`` Growtokens short.");
            pPlayer->PlaySFX("bleep_fail.wav");
            return false;
        }
    }
    else
    {
        if (cost > pPlayer->GetGems())
        {
            pPlayer->SendOnStorePurchaseResult("You can't afford " + pStoreEntry->name + "! You're `$" +
                                               ToString(cost - pPlayer->GetGems()) + "`` Gems short.");
            pPlayer->PlaySFX("bleep_fail.wav");
            return false;
        }
    }

    if (pStoreEntry->items.empty() && !isUpgradeBackpack)
    {
        pPlayer->SendOnStorePurchaseResult("Purchasing error, can't find your items!");
        pPlayer->PlaySFX("bleep_fail.wav");
        return false;
    }

    PlayerInventory& inventory = pPlayer->GetInventory();

    if (!isUpgradeBackpack)
    {
        uint32 slotNeeded = pStoreEntry->items.size();
        if (pStoreEntry->rule.randomCount > 0)
            slotNeeded = pStoreEntry->rule.randomCount;

        if (inventory.GetInventoryCapacity() < slotNeeded)
        {
            pPlayer->SendOnStorePurchaseResult("You'll need " + ToString(pStoreEntry->rule.randomCount) +
                                               " slots free to buy that");
            pPlayer->PlaySFX("bleep_fail.wav");
            return false;
        }
    }

    std::vector<int32> randItems;

    if (pStoreEntry->rule.randomCount > 0)
    {
        if (pStoreEntry->rule.randomCount > pStoreEntry->items.size())
        {
            pPlayer->SendOnStorePurchaseResult("Purchasing error, can't get your items!");
            pPlayer->PlaySFX("bleep_fail.wav");
            return false;
        }

        std::vector<int32> pool;
        for (int32 i = 0; i < pStoreEntry->items.size(); i += 2)
        {
            pool.push_back(i);
        }

        for (int32 i = 0; i < pStoreEntry->rule.randomCount; ++i)
        {
            int32 idx = RandomRangeInt(i, pool.size() - 1);

            std::swap(pool[i], pool[idx]);
            int32 base = pool[i];

            randItems.push_back(pStoreEntry->items[base]);
            randItems.push_back(pStoreEntry->items[base + 1]);
        }

        if (!inventory.CanAllItemsFit(randItems))
        {
            pPlayer->SendOnStorePurchaseResult("You can't fit all that in your backpack!");
            pPlayer->PlaySFX("bleep_fail.wav");
            return false;
        }
    }
    else if (!inventory.CanAllItemsFit(pStoreEntry->items))
    {
        pPlayer->SendOnStorePurchaseResult("You can't fit all that in your backpack!");
        pPlayer->PlaySFX("bleep_fail.wav");
        return false;
    }

    string purchaseResult;
    if (cost < 0)
    {
        pPlayer->ModifyInventoryItem(ITEM_ID_GROWTOKEN, cost);
        purchaseResult += "You've purchased " + pStoreEntry->name + " for `$" + ToString(cost) +
                          "`` Growtokens\nYou have `$" + ToString(inventory.GetCountOfItem(ITEM_ID_GROWTOKEN)) +
                          "`` Growtokens left.";
        LOGGER_LOG_INFO("Player %s (%d) bought %s from store %d tokens left", pPlayer->GetRawName().c_str(),
                        pPlayer->GetUserID(), pStoreEntry->code.c_str(), inventory.GetCountOfItem(ITEM_ID_GROWTOKEN));
    }
    else
    {
        pPlayer->ModifyGems(-cost, true);
        purchaseResult += "You've purchased " + pStoreEntry->name + " for `$" + ToString(cost) +
                          "`` Gems\nYou have `$" + ToString(pPlayer->GetGems()) + "`` Gems left.";
        LOGGER_LOG_INFO("Player %s (%d) bought %s from store %d gems left", pPlayer->GetRawName().c_str(),
                        pPlayer->GetUserID(), pStoreEntry->code.c_str(), pPlayer->GetGems());
    }
    pPlayer->SendOnConsoleMessage(purchaseResult);

    if (pStoreEntry->rule.randomCount > 0)
    {
        purchaseResult += "\n\n`5Received: ``" + GivePurchasedItemsAndGetGivensAsStr(pPlayer, randItems, pStoreEntry);
    }
    else
    {
        purchaseResult +=
            "\n\n`5Received: ``" + GivePurchasedItemsAndGetGivensAsStr(pPlayer, pStoreEntry->items, pStoreEntry);
    }

    pPlayer->SendOnStorePurchaseResult(purchaseResult);
    pPlayer->PlaySFX("piano_nice.wav");
    return true;
}

string StoreManager::GivePurchasedItemsAndGetGivensAsStr(GamePlayer* pPlayer, const std::vector<int32>& items,
                                                         StoreEntry* pStoreEntry)
{
    if (!pPlayer || !pStoreEntry)
        return "";

    PlayerInventory& inventory = pPlayer->GetInventory();
    if (pStoreEntry->code == "upgrade_backpack")
    {
        inventory.SetInventoryCapacity(inventory.GetInventoryCapacity() + 10);
        pPlayer->SendInventoryPacket();
        return "Backpack Upgrade";
    }

    if (items.size() % 2 != 0)
        return "";

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    string out;

    for (uint16 i = 0; i + 1 < items.size(); i += 2)
    {
        int32 itemID = items[i + 1];
        int32 count = items[i];

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
        if (!pItem)
            continue;

        if (!out.empty())
            out += ", ``";
        out += "``" + pItem->name;

        if (pItem->id == ITEM_ID_GEMS)
        {
            pPlayer->ModifyGems(count, true);
        }
        else
        {
            inventory.AddItem(itemID, count, pPlayer);
        }
    }

    return out;
}

StoreEntry* StoreManager::GetStoreEntryByCodeFromTab(const string& entryCode, eStoreTab storeTab)
{
    if (storeTab >= STORE_TAB_COUNT || entryCode.empty())
        return nullptr;

    for (auto& entry : m_storeEntries[(int32)storeTab])
    {
        if (entry.code == entryCode)
            return &entry;
    }

    return nullptr;
}

StoreEntry* StoreManager::GetStoreEntryByCode(const string& entryCode)
{
    if (entryCode.empty())
        return nullptr;

    uint32 hash = HashString(entryCode);

    auto it = m_storeSearch.find(hash);
    if (it != m_storeSearch.end())
    {
        if (it->second)
        {
            if (!it->second->isTab && it->second->code == entryCode)
                return it->second;
            else if (it->second->isTab)
                return it->second;
        }

        return nullptr;
    }

    return nullptr;
}

void StoreManager::BuildStore(GamePlayer* pPlayer, DialogBuilder& db, eStoreTab storeTab)
{
    if (storeTab >= STORE_TAB_COUNT)
        return;

    string welcomeDesc;
    if (storeTab == STORE_TAB_MAIN_MENU)
    {
        welcomeDesc = "Welcome to the `2Growtopia Store``! Select the item you'd like more info on.`o ";
        if (pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
        {
            welcomeDesc += "`wThanks for being a supporter of Growtopia!\n";
        }
        else
        {
            welcomeDesc += "`wWant to get `5Supporter`` status? Any Gem purchase (or `57,000`` Gems earne d with free "
                           "`5Tapjoy`` offers) will make you one. You'll get new skin colors,  the `5Recycle`` tool "
                           "to convert unwanted items into Gems, and more bonuses!";
        }
    }
    else if (storeTab == STORE_TAB_LOCKS_MENU)
    {
        welcomeDesc = "`2Locks And Stuff!``  Select the item you'd like more info on, or BACK to go back.";
    }
    else if (storeTab == STORE_TAB_ITEMPACK_MENU)
    {
        welcomeDesc = "`2Item Packs!``  Select the item you'd like more info on, or BACK to go back.";
    }
    else if (storeTab == STORE_TAB_BIGITEMS_MENU)
    {
        welcomeDesc = "`2Awesome Items!``  Select the item you'd like more info on, or BACK to go back.";
    }
    else if (storeTab == STORE_TAB_WEATHER_MENU)
    {
        welcomeDesc = "`2Weather Machines!``  Select the item you'd like more info on, or BACK to go back.";
    }
    else if (storeTab == STORE_TAB_TOKEN_MENU)
    {
        welcomeDesc = "You earn Growtokens from Crazy Jim and Sales-Man. Select the item you'd like more info on, or "
                      "BACK to go back.";
    }

    db.SetDescriptionText(welcomeDesc);

    for (uint8 i = 0; i < STORE_TAB_COUNT; ++i)
    {
        if (m_storeEntries[i].empty())
            continue;

        StoreEntry& tab = m_storeEntries[i][0];
        db.AddTabButton(tab.code, tab.name, tab.banner, tab.description, i == (int32)storeTab, tab.posY);
    }

    for (auto& entry : m_storeEntries[(int32)storeTab])
    {
        BuildStoreEntry(pPlayer, db, &entry);
    }
}

void StoreManager::BuildStoreEntry(GamePlayer* pPlayer, DialogBuilder& db, StoreEntry* pEntry)
{
    if (!pEntry || pEntry->isTab)
        return;

    if (pEntry->rule.disabled)
        return;

    uint64 epochNow = Time::GetTimeSinceEpoch();
    if (pEntry->rule.startTime > 0 && epochNow < pEntry->rule.startTime)
        return;

    if (pEntry->rule.endTime > 0 && epochNow > pEntry->rule.endTime)
        return;

    if (!pEntry->holidayKey.empty() && !GetHolidayManager()->IsHolidayActive(pEntry->holidayKey))
        return;

    int32 cost = pEntry->GetCost(pPlayer);
    if (cost == 0)
        return;

    if (pEntry->code == "upgrade_backpack" && pPlayer->GetInventory().GetInventoryCapacity() >= 396)
        return;

    db.AddButton(pEntry->code, pEntry->name, pEntry->banner, pEntry->description, pEntry->posX, pEntry->posY, cost);
}

StoreEntry* StoreManager::GetEntryByCodeRaw(const string& entryCode)
{
    if (entryCode.empty())
        return nullptr;

    for (auto& tab : m_storeEntries)
    {
        for (auto& entry : tab)
        {
            if (entry.code == entryCode)
                return &entry;
        }
    }

    return nullptr;
}

void StoreManager::HashEntries()
{
    for (auto& tab : m_storeEntries)
    {
        for (auto& entry : tab)
        {
            if (entry.code.empty())
                continue;

            if (entry.isTab)
            {
                usize pos = entry.code.find("_menu");

                if (pos != string::npos)
                {
                    m_storeSearch.insert_or_assign(HashString(entry.code.substr(0, pos)), &entry);
                }
            }

            m_storeSearch.insert_or_assign(HashString(entry.code), &entry);
        }
    }
}

StoreManager* GetStoreManager()
{
    return StoreManager::GetInstance();
}
