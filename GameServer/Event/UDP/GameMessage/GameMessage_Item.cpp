#include "GameMessage_Item.h"
#include "../../../Dialog/GameDialogs.h"
#include "../../../Store/StoreManager.h"
#include "Item/ItemInfoManager.h"
#include "Packet/NetPacket.h"

void GameMessage_Drop(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if (auto pItemID = packet.Find("itemID"_hash))
    {
        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        pPlayer->DropItem(itemID, 1, true);
    }
}

void GameMessage_Trash(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pItemID = packet.Find("itemID"_hash))
    {
        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        TrashDialog::Request(pPlayer, itemID);
    }
}

void GameMessage_RefreshItemData(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    pPlayer->SendOnConsoleMessage("One moment updating item data...");

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    ItemsClientData* clientData =
        GetItemInfoManager()->GetClientData(loginDetail.platformType, loginDetail.gameVersion);
    if (!clientData->pItemData)
    {
        pPlayer->SendOnConsoleMessage("Someting went wrong while sending updates");
        pPlayer->LogOff(true, false, true);
        LOGGER_LOG_WARN("Not sending file update data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA;
    gamePacket.field_4 = -1;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;

    if (clientData->compressSize != 0)
    {
        gamePacket.field_7 = clientData->size;
        gamePacket.extraDataSize = clientData->compressSize;
    }
    else
    {
        gamePacket.extraDataSize = clientData->size;
    }

    SendUDPItemDataPacket(pPlayer->GetNetID(), loginDetail.platformType, loginDetail.gameVersion, &gamePacket);
}

void GameMessage_Buy(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItem = packet.Find("item"_hash);
    if (!pItem)
        return;

    if (pItem->valueSize > 50)
        return;

    StoreManager* pStoreMgr = GetStoreManager();

    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(pItem->GetString());
    if (!pStoreEntry)
        return;

    if (pStoreEntry->isTab)
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
    else
        pStoreMgr->PurchaseItem(pPlayer, pStoreEntry->code);
}

void GameMessage_Store(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pLocation = packet.Find("location"_hash);
    if (!pLocation)
        return;

    if (pLocation->valueSize > 50)
        return;

    StoreManager* pStoreMgr = GetStoreManager();
    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(pLocation->GetString());

    if (!pStoreEntry)
        pStoreMgr->NavigatePlayer(pPlayer, STORE_TAB_MAIN_MENU);
    else
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
}

void GameMessage_Info(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if (!pItemID)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (pPlayer->GetInventory().GetCountOfItem(pItem->id) < 1)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o');

    if (pPlayer->GetLoginDetail().protocol >= 32) // not sure, but its close enough
        db.AddLabelWithEleIcon("`wAbout " + pItem->name, pItem->id, pItem->element, true);
    else
        db.AddLabelWithIcon("`wAbout " + pItem->name, pItem->id, true);

    db.AddSpacer();

    if (pItem->type == ITEM_TYPE_SEED)
    {
        ItemInfo* pBaseItem = GetItemInfoManager()->GetItemByID(SEED_TO_ITEM_ID(pItem->id));
        if (pBaseItem)
            db.AddTextBox("Plant this seed to grow a `w" + pBaseItem->name + " ``Tree.");
    }
    else
        db.AddTextBox(pItem->description);

    db.AddSpacer();

    string properties;
    if (pItem->rarity < 999)
    {
        properties += "Rarity: `w" + ToString(pItem->rarity) + "``<CR>";
    }

    if (pPlayer->GetLoginDetail().protocol < 32 && pItem->element != ITEM_ELEMENT_NONE)
    {
        properties += "<CR>Item Element: ";
        if (pItem->element == ITEM_ELEMENT_EARTH)
            properties += "`2Earth";
        else if (pItem->element == ITEM_ELEMENT_FIRE)
            properties += "`4Fire";
        else if (pItem->element == ITEM_ELEMENT_AIR)
            properties += "`9Air";
        else if (pItem->element == ITEM_ELEMENT_WATER)
            properties += "`1Water";
    }

    if (pItem->type == ITEM_TYPE_FIST)
    {
        uint32 punchCount = pPlayer->GetProgressData().GetProgress(PLAYER_PROGRESS_PUNCH_COUNT);
        properties += "You've punched `w" + ToString(punchCount) + "`` times.";
    }

    if (pItem->type == ITEM_TYPE_CLOTHES)
        properties += "Everybody has one of these, it's not that special even though it looks cool.";

    if (pItem->type == ITEM_TYPE_LOCK)
        properties += "A lock makes it so only you (and designated friends) can edit an area.";

    if (pItem->type != ITEM_TYPE_SEED)
    {
        if (ItemInfo* pSeedItem = GetItemInfoManager()->GetItemByID(ITEM_TO_SEED_ID(pItem->id)))
        {
            if (pSeedItem->seed1 == ITEM_ID_BLANK)
                properties += "<CR>This item can't be spliced.";
            else
            {
                ItemInfo* pSeed1Info = GetItemInfoManager()->GetItemByID(pSeedItem->seed1);
                ItemInfo* pSeed2Info = GetItemInfoManager()->GetItemByID(pSeedItem->seed2);

                if (pSeed1Info && pSeed2Info)
                {
                    if (pItem->HasFlag(ITEM_FLAG_SEEDLESS))
                    {
                        properties += "<CR>To grow, splice a `w" + pSeed1Info->name + "`` with a `w" +
                                      pSeed2Info->name + "``.<CR>";
                    }
                    else
                    {
                        properties += "<CR>To grow, plant a `w" + pSeedItem->name + "``. (Or splice a `w" +
                                      pSeed1Info->name + "`` with a `w" + pSeed2Info->name + "``)<CR>";
                    }
                }
            }
        }
    }

    if (pItem->type == ITEM_TYPE_DICE)
        properties += "<CR>`1This item has no use... by itself.``";

    if (pItem->type == ITEM_TYPE_PETFISH)
        properties +=
            "<CR>`1This is a fish. It can only be placed in a Fish Tank. You can't divide fish into pieces.``";

    if (pItem->HasFlag(ITEM_FLAG_SEEDLESS))
        properties += "<CR>`1This item never drops any seeds.``";

    if (pItem->HasFlag(ITEM_FLAG_NOSELF))
    {
        if (pItem->type == ITEM_TYPE_CONSUMABLE)
            properties += "<CR>`1This item can't be used on yourself.``";
        else
            properties += "<CR>`4This weapon can kill zombies during a Pandemic!``";
    }

    if (pItem->HasFlag(ITEM_FLAG_WORLDLOCKED))
        properties += "<CR>`1This item can only be used in World-Locked worlds.``";

    if (pItem->HasFlag(ITEM_FLAG_RANDGROW))
        properties += "<CR>`1A tree of this type can bear surprising fruit!``";

    if (pItem->HasFlag(ITEM_FLAG_PUBLIC))
        properties += "<CR>`1This item is PUBLIC: Even if it's locked, anyone can smash it.``";

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        properties += "<CR>`1This item cannot be dropped or traded.``";

    if (pItem->HasFlag(ITEM_FLAG_PERMANENT))
    {
        if (pItem->HasFlag(ITEM_FLAG_AUTOPICKUP))
            properties +=
                "<CR>`1This item can't be destroyed - smashing it will return it to your backpack if you have room!``";
        else
            properties += "<CR>`1This item can't be destroyed - smashing it will always yield a new one.``";
    }

    if (pItem->HasFlag(ITEM_FLAG_FLIPPABLE))
        properties += "<CR>`1This item can be placed in two directions, depending on the direction you're facing.``";

    if (pItem->HasFlag(ITEM_FLAG_EDITABLE))
        properties += "<CR>`1This item has special properties you can adjust with the Wrench.``";

    if (pItem->type == ITEM_TYPE_ARTIFACT)
        properties += "<CR>`1This item can be upgraded.";

    if (!properties.empty())
        db.AddTextBox(properties).AddSpacer();

    db.EmbedData("itemID", ToItemClientID(pItem->id)).EndDialog("continue", "OK", "");
    pPlayer->SendOnDialogRequest(db.Get());
}
