#pragma once

#include "../Item/ItemUtils.h"
#include "../Memory/MemoryBuffer.h"
#include "../Precompiled.h"

#define INVENTORY_DEFAULT_CAPACITY 16
#define INVENTORY_PROTOCOL_BIG_BACKPACK 94
#define INVENTORY_VERSION_LEGACY 0
#define INVENTORY_VERSION_BIG 1
#define INVENTORY_LEGACY_MAX_ITEMS 245

struct InventoryItemInfo
{
    int16 id = 0;
    uint8 count = 0;
    uint8 flags = 0;

    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
};

enum eInventoryFlags
{
    INVENTORY_FLAG_NONE,
    INVENTORY_FLAG_WEARING = 1 << 0,
    INVENTORY_FLAG_IS_SEED = 1 << 1,
    INVENTORY_FLAG_NEVER_PICKED_UP = 1 << 2
};

enum eInventorySwapResult
{
    INVENTORY_SWAP_OK,
    INVENTORY_SWAP_NOT_LEGACY,
    INVENTORY_SWAP_HIDDEN_NOT_FOUND,
    INVENTORY_SWAP_VISIBLE_NOT_FOUND,
    INVENTORY_SWAP_VISIBLE_PINNED
};

class Player;

class PlayerInventory
{
public:
    PlayerInventory();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    InventoryItemInfo* GetItemByID(int32 itemID);

    uint8 AddItem(int32 itemID, uint8 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(int32 itemID, int16 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(int32 itemID, Player* pPlayer = nullptr);

    const int16* GetClothes() const { return m_clothes; }
    int16 GetClothByPart(eBodyPart bodyPart) const { return m_clothes[bodyPart]; }
    void SetClothByPart(int32 itemID, uint8 bodyPart, Player* pPlayer = nullptr);
    bool IsWearingItem(int32 itemID);

    bool HaveRoomForItem(int32 itemID, uint8 itemCount);
    uint32 GetFitItemCount(int32 itemID);
    bool CanAllItemsFit(const std::vector<int32>& items);

    uint8 GetCountOfItem(int32 itemID);

    void UpdateInventory(Player* pPlayer, int32 itemID, uint8 count, bool added);
    void RemoveFromQuickSlots(int32 itemID);

    bool IsWearingPlayMod(int32 playModType);

    uint32 GetMemEstimate(bool database);
    void SetVersion(uint32 protocol);
    uint8 GetVersion() const { return m_version; }
    bool IsLegacy() const { return m_version == INVENTORY_VERSION_LEGACY; }

    uint32 GetOverflowCount() const;
    InventoryItemInfo* GetOverflowItem(uint32 overflowIndex);
    eInventorySwapResult SwapWithOverflow(int32 visibleItemID, int32 hiddenItemID, Player* pPlayer);

    void SetInventoryCapacity(uint32 capacity) { m_capacity = capacity; }
    uint32 GetInventoryCapacity() const { return m_capacity; }
    uint32 GetItemsSize() const { return m_items.size(); }
    uint32 GetInventorySpace() const { return m_capacity > m_items.size() ? m_capacity - m_items.size() : 0; }

private:
    int32 GetItemIndex(int32 itemID) const;
    bool IsPinned(const InventoryItemInfo& item) const;
    bool IsClientVisible(int32 index) const { return !IsLegacy() || index < INVENTORY_LEGACY_MAX_ITEMS; }
    void SwapVisibleWithHidden(int32 visibleIdx, int32 hiddenIdx, Player* pPlayer);
    void EnsureVisible(int32 itemID, Player* pPlayer);

private:
    uint8 m_version;
    uint32 m_capacity;

    std::vector<InventoryItemInfo> m_items;
    int16 m_clothes[BODY_PART_SIZE];
    int16 m_quickSlots[4];
};