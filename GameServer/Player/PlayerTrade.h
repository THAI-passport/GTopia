#pragma once

#include "Packet/PacketUtils.h"
#include "Precompiled.h"
#include "Utils/DialogBuilder.h"
#include "Utils/Timer.h"

struct TradeItemInfo
{
    int32 itemID = 0;
    int32 count = 0;
};

class InventoryItemInfo;
class GamePlayer;
class ItemInfo;

class PlayerTrade
{
public:
    PlayerTrade(GamePlayer* pPlayer);
    ~PlayerTrade();

public:
    void StartTradingWith(GamePlayer* pPartner);
    void OnTradeItem(int32 itemID, int32 count, bool askQuantity);
    void OnAddItem(ItemInfo* pItem, InventoryItemInfo* pInvItem, int32 count);
    void OnRemoveItem(int32 itemID);
    bool RemoveFromItems(int32 itemID);

    void SendTradeStatus(bool dealChanged, bool resetAccept, bool forceNotify);
    string GetStatusText(bool resetLocks);
    void SetDelayToAcceptButton(int32 delayMS = 2000);

    void SetAccept(bool accepted, ParsedTextPacket<38>* packet = nullptr);
    void SendDoubleAcceptDialog();
    bool ValidateExpectedTradeItemsFromText(ParsedTextPacket<38>& packet);
    string CheckTradingItemsAndGetError();
    bool CanHandleAllItemStacks(GamePlayer* pTargetPlayer, string& outErrorItem, int32& outItemCount);
    bool HasEnoughBackpackSpaceAfterTheTrade();
    void ListItemsInDialog(DialogBuilder& db);

    int32 TransferItems(int32 delayMS);
    void DoFinalTransaction();
    void OnEndTrade(GamePlayer* pPlayer);
    void EndTrade();
    void AbortTradeWithMessage(const string& message);
    void KillTrade();
    void EndTradeWithMessage(const string& message);

    bool IsBothAcceptedTrade();
    bool IsEnoughTimeElapsedSinceLastAction();
    bool IsTrading() const { return (m_pPartner != nullptr); }
    bool IsAccepted() const { return m_accepted; }
    void SetLock(bool locked) { m_locked = locked; }

    uint32 GetTradingItemsCount() const { return m_items.size(); }
    const std::vector<TradeItemInfo>& GetItems() const { return m_items; }
    GamePlayer* GetPartner() const { return m_pPartner; }

private:
    GamePlayer* m_pPlayer;
    GamePlayer* m_pPartner;

    bool m_locked;
    bool m_accepted;
    Timer m_lastActionTimer;
    Timer m_acceptDelayTimer;
    std::vector<TradeItemInfo> m_items;
};