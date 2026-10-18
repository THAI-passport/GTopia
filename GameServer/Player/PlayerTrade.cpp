#include "PlayerTrade.h"
#include "../Dialog/GameDialogs.h"
#include "../World/WorldManager.h"
#include "GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include <sstream>

PlayerTrade::PlayerTrade(GamePlayer* pPlayer)
    : m_pPlayer(pPlayer), m_pPartner(nullptr), m_accepted(false), m_locked(false)
{
    m_lastActionTimer.Set(0);
    m_acceptDelayTimer.Set(0);
}

PlayerTrade::~PlayerTrade() {}

void PlayerTrade::StartTradingWith(GamePlayer* pPartner)
{
    if (!m_pPlayer || !pPartner || pPartner == m_pPlayer)
        return;

    KillTrade();

    if (pPartner->GetTradeManager().IsTrading() && pPartner->GetTradeManager().GetPartner() != m_pPlayer)
    {
        m_pPlayer->SendOnTextOverlay(pPartner->GetRawName() + " is trading with someone else now!");
        return;
    }

    m_pPartner = pPartner;

    if (pPartner->GetTradeManager().GetPartner() == m_pPlayer)
    {
        SendTradeStatus(true, false, false);
        pPartner->GetTradeManager().SendTradeStatus(true, false, false);
    }
    else
    {
        string tradeMsg =
            "`#TRADE ALERT:`` `w" + m_pPlayer->GetRawName() +
            "`` wants to trade with you! To start, use the `wWrench`` on that person's wrench icon, or type `w/trade " +
            m_pPlayer->GetRawName();
        pPartner->SendOnConsoleMessage(tradeMsg);
        pPartner->PlaySFX("audio/cash_register.wav");
    }
}

void PlayerTrade::OnTradeItem(int32 itemID, int32 count, bool askQuantity)
{
    if (!m_pPlayer)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    InventoryItemInfo* pInvItem = m_pPlayer->GetInventory().GetItemByID(pItem->id);
    if (!pInvItem)
        return;

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE) || pItem->IsUnlimited())
    {
        m_pPlayer->SendOnTextOverlay("You'd be sorry if you lost that!");
        m_pPlayer->PlaySFX("cant_place_tile.wav");
        return;
    }

    if (pInvItem->count < count)
        return;

    if (pItem->type == ITEM_TYPE_PETFISH)
    {
        askQuantity = false;
        count = pInvItem->count;
    }

    if (!askQuantity || count != 1 || pInvItem->count < 2)
    {
        OnAddItem(pItem, pInvItem, count);
    }
    else
    {
        TradeDialog::Request(m_pPlayer, pInvItem);
    }
}

void PlayerTrade::OnAddItem(ItemInfo* pItem, InventoryItemInfo* pInvItem, int32 count)
{
    if (!pItem || !pInvItem || !m_pPlayer)
        return;

    if (!IsTrading() || IsBothAcceptedTrade())
        return;

    if (m_items.size() >= 4)
    {
        m_pPlayer->SendOnTextOverlay("Can't add another item!");
        return;
    }

    if (!IsEnoughTimeElapsedSinceLastAction())
        return;

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        m_pPlayer->SendOnTextOverlay("You can't trade that! You shouldn't even have that!");
        return;
    }

    if (pItem->id == ITEM_ID_WORLD_KEY)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
        if (!pWorld)
        {
            m_pPlayer->SendOnTextOverlay("Huh, There are no worlds found that owns the key");
            return;
        }

        if (pWorld->IsThereAnyUncoveredObject())
        {
            m_pPlayer->SendOnTextOverlay("There are floating items that are unblocked or in treasure chests!");
            return;
        }

        WorldTileManager* pTileManager = pWorld->GetTileManager();
        if (pTileManager->GetTileInfoFlaggedWith(ITEM_FLAG_UNTRADEABLE))
        {
            m_pPlayer->SendOnTextOverlay("There are Untradeable blocks in the world!");
            return;
        }

        if (pTileManager->GetTileInfoByItemID(ITEM_ID_SAFE_VAULT))
        {
            m_pPlayer->SendOnTextOverlay("There is a Safe Vault placed in this world!");
            return;
        }
    }

    RemoveFromItems(pItem->id);
    m_items.emplace_back(TradeItemInfo{pItem->id, count});

    if (m_pPartner && m_pPartner->GetTradeManager().GetPartner() == m_pPlayer)
    {
        string countStr = ToString(count);
        if (pItem->type == ITEM_TYPE_PETFISH)
            countStr += "lb.";

        string changeStatus =
            "`1TRADE CHANGE: `` " + m_pPlayer->GetDisplayName(true) + " added `w" + countStr + "`` " + pItem->name;
        m_pPartner->SendOnConsoleMessage(changeStatus);
    }

    SendTradeStatus(true, true, true);
}

void PlayerTrade::OnRemoveItem(int32 itemID)
{
    if (IsTrading() && !IsBothAcceptedTrade() && IsEnoughTimeElapsedSinceLastAction())
    {
        if (RemoveFromItems(itemID))
        {
            SendTradeStatus(true, true, true);
        }
    }
}

bool PlayerTrade::RemoveFromItems(int32 itemID)
{
    for (int32 i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].itemID == ITEM_ID_BLANK || m_items[i].itemID != itemID)
            continue;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_items[i].itemID);
        if (m_pPlayer && m_pPartner && pItem)
        {
            PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
            if (partnerTradeMgr.GetPartner() == m_pPlayer)
            {
                string countStr = ToString(m_items[i].count);
                if (pItem->type == ITEM_TYPE_PETFISH)
                    countStr += "lb.";

                string changeStatus =
                    "`1TRADE CHANGE: `` " + m_pPlayer->GetRawName() + " removed `w" + countStr + "`` " + pItem->name;
                m_pPartner->SendOnConsoleMessage(changeStatus);
            }
        }

        m_items.erase(m_items.begin() + i);
        return true;
    }

    return false;
}

bool PlayerTrade::IsEnoughTimeElapsedSinceLastAction()
{
    if (!m_lastActionTimer.IsPassed())
    {
        if (m_pPlayer)
        {
            m_pPlayer->SendOnTextOverlay("Slow down!  Please wait a second between adding and removing items");
        }
        return false;
    }

    m_lastActionTimer.Set(500);
    return true;
}

void PlayerTrade::SetDelayToAcceptButton(int32 delayMS)
{
    m_acceptDelayTimer.Set(delayMS);
    m_accepted = false;

    if (!m_pPlayer)
        return;

    string headerText = "`o" + m_pPlayer->GetDisplayName(true) + "'s offer.``";

    m_locked = true;
    m_pPlayer->SendOnTradeStatus(m_pPlayer->GetNetID(), "", headerText, GetStatusText(false));

    m_locked = false;
    m_pPlayer->SendOnTradeStatus(m_pPlayer->GetNetID(), "", headerText, GetStatusText(true));
}

void PlayerTrade::SendTradeStatus(bool dealChanged, bool resetAccept, bool forceNotify)
{
    if (!m_pPlayer)
        return;

    if (dealChanged && m_accepted)
    {
        m_accepted = false;
        m_pPlayer->SendOnTextOverlay("The deal has changed");
        m_pPlayer->PlaySFX("audio/tile_removed.wav");
    }

    string headerText = "`o" + m_pPlayer->GetDisplayName(true) + "'s offer.``";

    m_pPlayer->SendOnTradeStatus(m_pPlayer->GetNetID(), "", headerText, GetStatusText(dealChanged));

    if (m_pPartner)
    {
        PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
        if (partnerTradeMgr.GetPartner() == m_pPlayer)
        {
            if (dealChanged)
            {
                if (partnerTradeMgr.IsAccepted() || forceNotify)
                {
                    m_pPartner->SendOnTextOverlay("The deal has changed.");
                    m_pPartner->PlaySFX("audio/tile_removed.wav");
                    m_pPlayer->SendOnTextOverlay("The deal has changed.");
                    m_pPlayer->PlaySFX("audio/tile_removed.wav");
                }
                partnerTradeMgr.SetDelayToAcceptButton();
            }
            else if (resetAccept)
            {
                partnerTradeMgr.SetDelayToAcceptButton();
                if (forceNotify)
                {
                    m_pPartner->SendOnTextOverlay("The deal has changed.");
                    m_pPartner->PlaySFX("audio/tile_removed.wav");
                    m_pPlayer->SendOnTextOverlay("The deal has changed.");
                    m_pPlayer->PlaySFX("audio/tile_removed.wav");
                }
            }

            m_pPartner->SendOnTradeStatus(m_pPlayer->GetNetID(), "", headerText, GetStatusText(dealChanged));
        }
    }
}

string PlayerTrade::GetStatusText(bool resetLocks)
{
    string res;

    for (int32 i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].itemID == ITEM_ID_BLANK)
            continue;

        res += "add_slot|" + ToString(ToItemClientID(m_items[i].itemID)) + "|" + ToString(m_items[i].count) + "\n";
    }

    res += "locked|" + ToString(m_locked ? 1 : 0) + "\n";

    if (resetLocks)
    {
        res += "reset_locks|1\n";
    }

    res += "accepted|" + ToString(m_accepted ? 1 : 0) + "\n";
    return res;
}

bool PlayerTrade::ValidateExpectedTradeItemsFromText(ParsedTextPacket<38>& packet)
{
    ItemInfoManager* pItemManager = GetItemInfoManager();
    int32 currentIndex = 0;
    int32 itemCount = 0;

    for (auto& field : packet.FindAll("item"_hash))
    {
        if (currentIndex >= m_items.size())
            return false;

        uint32 itemID = 0;
        if (field.GetUIntAt(1, itemID) != TO_INT_SUCCESS)
            return false;

        uint32 count = 0;
        if (field.GetUIntAt(2, count) != TO_INT_SUCCESS)
            return false;

        ItemInfo* pItem = pItemManager->GetItemByID(itemID);
        if (!pItem)
            return false;

        if (m_items[currentIndex].itemID != pItem->id || m_items[currentIndex].count != count)
            return false;

        currentIndex++;
    }

    if (currentIndex != m_items.size())
        return false;

    return true;
}

string PlayerTrade::CheckTradingItemsAndGetError()
{
    if (!m_pPlayer)
        return "`4Something bad happened.``";

    for (auto& item : m_items)
    {
        InventoryItemInfo* pInvItem = m_pPlayer->GetInventory().GetItemByID(item.itemID);
        if (!pInvItem || pInvItem->count < item.count)
            return "`4Oops Items missing from trade!``";

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(item.itemID);
        if (!pItem)
            return "`4Oops Items missing from database!``";

        if (item.itemID == ITEM_ID_WORLD_KEY)
        {
            World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
            if (!pWorld)
                return "`4This world doesn't exist!?!``";

            TileInfo* pWorldLock = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
            if (!pWorldLock)
                return "`4Oop - This world isn't locked!``";

            TileExtra_Lock* pTileExtra = pWorldLock->GetExtra<TileExtra_Lock>();
            if (!pTileExtra)
                return "`4The lock is bugged or doesn't exist.``";

            if (pTileExtra->ownerID != m_pPlayer->GetUserID())
                return "`4Oop - You don't own this world!``";

            if (pWorld->GetTileManager()->GetLockCount() != 1)
                return "`4Oop - World has extra locks on it. Remove them first!``";

            if (!pTileExtra->accessList.empty())
                return "`4Oop - World needs all co-owners removed from World Lock first``";

            if (pWorldLock->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
                return "`4Oop - Can't trade a world that's set to Public!``";

            if (pWorld->IsThereAnyUncoveredObject())
                return "`4Oop - Can't trade a world with floating items that are unblocked or in treasure chests!``";

            if (pWorld->GetTileManager()->GetTileInfoFlaggedWith(ITEM_FLAG_UNTRADEABLE))
                return "`4Oop - Can't trade a world with Untradeable blocks!``";

            if (pWorld->GetTileManager()->GetTileInfoByItemID(ITEM_ID_SAFE_VAULT))
                return "`4Oop - Can't trade a world with any Safe Vaults placed!``";
        }
        else
        {
            if (pItem->type == ITEM_TYPE_PETFISH && m_pPartner)
            {
                if (m_pPartner->GetInventory().GetCountOfItem(item.itemID) > 0)
                    return "`4Oops - can't trade a fish to someone already holding that fish type!``";
            }

            if (m_pPartner)
            {
                if (m_pPartner->GetInventory().GetCountOfItem(item.itemID) + item.count > pItem->maxCanHold)
                    return "`4Oops - " + m_pPartner->GetRawName() + " needs more backpack space.``";
            }
        }
    }

    return "";
}

bool PlayerTrade::CanHandleAllItemStacks(GamePlayer* pTargetPlayer, string& outErrorItem, int32& outItemCount)
{
    if (!pTargetPlayer)
        return false;

    for (auto& item : m_items)
    {
        if (item.itemID == ITEM_ID_BLANK)
            continue;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(item.itemID);
        if (!pItem)
            continue;

        int32 currentCount = pTargetPlayer->GetInventory().GetCountOfItem(item.itemID);
        int32 totalCount = currentCount + item.count;

        if (pItem->type == ITEM_TYPE_PETFISH && currentCount > 0)
        {
            outErrorItem = pItem->name;
            outItemCount = -1;
            return false;
        }

        if (totalCount > pItem->maxCanHold)
        {
            outErrorItem = pItem->name;
            outItemCount = totalCount;
            return false;
        }
    }
    return true;
}

bool PlayerTrade::HasEnoughBackpackSpaceAfterTheTrade()
{
    if (!m_pPlayer || !m_pPartner)
        return false;

    int32 currentItems = m_pPlayer->GetInventory().GetItemsSize();
    int32 maxSlots = m_pPlayer->GetInventory().GetInventoryCapacity();
    int32 partnerTradingCount = m_pPartner->GetTradeManager().GetTradingItemsCount();

    return (maxSlots - currentItems + 4 - partnerTradingCount) >= 0;
}

void PlayerTrade::SetAccept(bool accepted, ParsedTextPacket<38>* packet)
{
    if (!m_pPlayer || !m_pPartner)
        return;

    PlayerTrade& partnerTrade = m_pPartner->GetTradeManager();

    if (accepted)
    {
        if (!m_acceptDelayTimer.IsPassed())
        {
            SetDelayToAcceptButton();
            return;
        }

        if (m_accepted != accepted)
        {
            if (partnerTrade.IsAccepted())
            {
                if (packet && !partnerTrade.ValidateExpectedTradeItemsFromText(*packet))
                {
                    SendTradeStatus(true, false, false);
                    return;
                }

                string err = CheckTradingItemsAndGetError();
                if (!err.empty())
                {
                    m_pPlayer->SendOnTextOverlay(err);
                    m_pPartner->SendOnTextOverlay(err);
                    return;
                }

                string partnerErr = partnerTrade.CheckTradingItemsAndGetError();
                if (!partnerErr.empty())
                {
                    m_pPlayer->SendOnTextOverlay(partnerErr);
                    m_pPartner->SendOnTextOverlay(partnerErr);
                    return;
                }

                if (!HasEnoughBackpackSpaceAfterTheTrade())
                {
                    string msg = m_pPlayer->GetDisplayName(true) + " needs more backpack room first!";
                    m_pPlayer->SendOnTextOverlay(msg);
                    m_pPartner->SendOnTextOverlay(msg);
                    return;
                }

                if (!partnerTrade.HasEnoughBackpackSpaceAfterTheTrade())
                {
                    string msg = m_pPartner->GetDisplayName(true) + " needs more backpack room first!";
                    m_pPlayer->SendOnTextOverlay(msg);
                    m_pPartner->SendOnTextOverlay(msg);
                    return;
                }

                m_accepted = true;
                SendTradeStatus(false, false, false);
                SendDoubleAcceptDialog();
                partnerTrade.SendDoubleAcceptDialog();
            }
            else
            {
                m_accepted = true;

                bool partnerTradingWorldLock = false;
                for (auto& item : partnerTrade.GetItems())
                {
                    if (item.itemID == ITEM_ID_WORLD_KEY)
                    {
                        partnerTradingWorldLock = true;
                        break;
                    }
                }

                if (partnerTradingWorldLock)
                {
                    m_pPlayer->SendOnTextOverlay("`6[``you must wait for the other player to accept first`6]``");
                    SendDoubleAcceptDialog();
                    m_pPartner->SendOnTalkBubble("`6[``You must accept first because you're selling a World Key`6]``",
                                                 false);
                }
                else
                {
                    m_pPlayer->SendOnTalkBubble("`6[``Trade accepted, waiting for other player to accept`6]``", false);
                    m_pPartner->SendOnTalkBubble("`6[``Trade accepted by other player, waiting for you`6]``", false);
                }

                SendTradeStatus(false, false, false);
            }
        }
    }
    else
    {
        m_accepted = false;
        SendTradeStatus(false, true, false);
    }
}

void PlayerTrade::SendDoubleAcceptDialog()
{
    if (!m_pPlayer || !m_pPartner)
        return;

    DialogBuilder db;
    db.SetDefaultColor('`o');
    db.AddLabelWithIcon("`wTrade Confirmation``", ITEM_ID_DUMB_FRIEND, true);
    db.AddSpacer();
    db.AddTextBox("`4You'll give:``");
    db.AddSpacer();

    ListItemsInDialog(db);

    db.AddSpacer();
    db.AddTextBox("`2You'll get:``");
    db.AddSpacer();

    m_pPartner->GetTradeManager().ListItemsInDialog(db);

    db.AddSpacer();

    bool partnerTradingWorldLock = false;
    bool selfTradingWorldLock = false;

    for (auto& item : m_pPartner->GetTradeManager().GetItems())
    {
        if (item.itemID == ITEM_ID_WORLD_KEY)
            partnerTradingWorldLock = true;
    }

    for (auto& item : m_items)
    {
        if (item.itemID == ITEM_ID_WORLD_KEY)
            selfTradingWorldLock = true;
    }

    if (partnerTradingWorldLock)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
        string worldName = pWorld ? pWorld->GetWorldName() : "this world";

        db.AddTextBox("`4SCAM WARNING: ``You are buying this world, `#" + worldName +
                      "``. Don't buy a world just to get its rare items, because scammers can easily place secret "
                      "doors allowing someone to jump in and `4steal the items back`` after a trade!");
        db.AddSpacer();
        db.AddTextBox(
            "Also, all ratings will be removed from a world when it is traded. Don't buy a world for its ranking!");
        db.AddSpacer();
        db.AddTextBox("To be safe, only buy a world for its name and placed blocks, not loose items or high ratings. "
                      "Consider yourself warned!");
        db.AddSpacer();

        if (pWorld)
        {
            TileInfo* pLock = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
            if (pLock && pLock->GetExtra<TileExtra_Lock>())
            {
                db.AddTextBox("This world is locked by " + m_pPartner->GetRawName() + ".");
                db.AddSpacer();
            }
        }
    }

    if (selfTradingWorldLock)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
        string worldName = pWorld ? pWorld->GetWorldName() : "this world";

        db.AddTextBox("`4WARNING: ``You are about to sell your world `#" + worldName +
                      "`` - the world lock ownership will be transferred over to `w" + m_pPartner->GetRawName() +
                      "``.");
        db.AddSpacer();
    }

    db.AddButton("accept", "Do The Trade!");
    db.AddButton("back", "Cancel");
    db.EndDialog("trade_confirm", "", "");

    m_pPlayer->SendOnDialogRequest(db.Get());
}

void PlayerTrade::ListItemsInDialog(DialogBuilder& db)
{
    bool anyItemFound = false;

    for (auto& item : m_items)
    {
        if (item.itemID == ITEM_ID_BLANK)
            continue;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(item.itemID);
        if (!pItem)
            continue;

        string displayStr = "``(`w" + ToString(item.count);
        if (pItem->type == ITEM_TYPE_PETFISH)
            displayStr += " lb.";
        displayStr += "``) " + pItem->name;

        db.AddLabelWithIcon(displayStr, pItem->id);
        anyItemFound = true;
    }

    if (!anyItemFound)
        db.AddTextBox("`4Nothing``");
}

void PlayerTrade::DoFinalTransaction()
{
    if (!m_pPlayer || !m_pPartner || !IsBothAcceptedTrade())
    {
        AbortTradeWithMessage("`4Trade Aborted for some reason!``");
        return;
    }

    string err = CheckTradingItemsAndGetError();
    if (!err.empty())
    {
        AbortTradeWithMessage(err);
        return;
    }

    string partnerErr = m_pPartner->GetTradeManager().CheckTradingItemsAndGetError();
    if (!partnerErr.empty())
    {
        AbortTradeWithMessage(partnerErr);
        return;
    }

    int32 delayMS = TransferItems(300);
    m_pPartner->GetTradeManager().TransferItems(delayMS + 300);

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
    if (pWorld)
    {
        pWorld->SendPlayPositionedToAll(m_pPlayer, "audio/keypad_hit.wav");
    }

    EndTradeWithMessage("");
}

int32 PlayerTrade::TransferItems(int32 delayMS)
{
    if (!m_pPlayer || !m_pPartner)
        return delayMS;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
    if (!pWorld)
        return delayMS;

    bool isFirstItem = true;
    string broadcastMsg = "`1" + m_pPlayer->GetDisplayName(true) + "`` traded ";

    for (int32 i = 0; i < m_items.size(); ++i)
    {
        TradeItemInfo& item = m_items[i];
        if (item.itemID == ITEM_ID_BLANK)
            continue;

        pWorld->ThrowItemToPlayerToPlayer(m_pPlayer, m_pPartner, item.itemID, item.count);
        pWorld->SendPlayPositionedToAll(m_pPlayer, "audio/change_clothes.wav");
        delayMS += 350;

        m_pPlayer->ModifyInventoryItem(item.itemID, -item.count);

        if (item.itemID == ITEM_ID_WORLD_KEY)
        {
        }
        else
        {
            m_pPartner->ModifyInventoryItem(item.itemID, item.count);
        }

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(item.itemID);
        if (pItem)
        {
            string itemAmountStr = ToString(item.count);
            if (pItem->type == ITEM_TYPE_PETFISH)
                itemAmountStr += "lb.";

            if (i == m_items.size() - 1 && !isFirstItem)
                broadcastMsg += " and ";
            else if (!isFirstItem)
                broadcastMsg += ", ";

            broadcastMsg += "[" + itemAmountStr + " " + pItem->name + "]``";
            isFirstItem = false;
        }
    }

    if (isFirstItem)
        broadcastMsg += "nothing";

    broadcastMsg += " to `1" + m_pPartner->GetDisplayName(true) + "``.``";
    pWorld->SendConsoleMessageToAll(broadcastMsg);

    return delayMS;
}

bool PlayerTrade::IsBothAcceptedTrade()
{
    if (!m_pPartner || !m_accepted)
        return false;

    return m_pPartner->GetTradeManager().IsAccepted();
}

void PlayerTrade::OnEndTrade(GamePlayer* pPlayer)
{
    if (!pPlayer || pPlayer != m_pPartner)
        return;

    m_pPlayer->SendOnTextOverlay(pPlayer->GetRawName() + " has canceled the trade");
    m_pPlayer->SendOnForceTradeEnded();
    EndTrade();
}

void PlayerTrade::EndTrade()
{
    m_pPartner = nullptr;
    m_accepted = false;
    m_locked = false;
    m_items.clear();
    m_lastActionTimer.Set(0);
    m_acceptDelayTimer.Set(0);
}

void PlayerTrade::EndTradeWithMessage(const string& message)
{
    if (!m_pPlayer)
        return;

    if (!message.empty())
    {
        m_pPlayer->SendOnTextOverlay(message);
        m_pPlayer->SendOnConsoleMessage(message);
    }
    m_pPlayer->SendOnForceTradeEnded();

    if (m_pPartner && m_pPartner->GetTradeManager().GetPartner() == m_pPlayer)
    {
        if (!message.empty())
        {
            m_pPartner->SendOnTextOverlay(message);
            m_pPartner->SendOnConsoleMessage(message);
        }
        m_pPartner->SendOnForceTradeEnded();
        m_pPartner->GetTradeManager().EndTrade();
    }

    EndTrade();
}

void PlayerTrade::AbortTradeWithMessage(const string& message)
{
    EndTradeWithMessage(message);
}

void PlayerTrade::KillTrade()
{
    if (m_pPartner)
    {
        m_pPartner->GetTradeManager().OnEndTrade(m_pPlayer);
    }

    EndTrade();
}