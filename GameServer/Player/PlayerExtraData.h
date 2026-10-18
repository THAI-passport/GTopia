#pragma once

#include "Memory/MemoryBuffer.h"
#include "Precompiled.h"
#include "Utils/Variant.h"

class GamePlayer;

enum ePlayerExtraKey : uint16
{
    PLAYER_EXTRA_BILLBOARD_ITEM_ID = 0,
    PLAYER_EXTRA_BILLBOARD_SHOW,
    PLAYER_EXTRA_BILLBOARD_PRICE,
    PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER,
    PLAYER_EXTRA_BILLBOARD_IS_BUY,

    PLAYER_EXTRA_COUNT
};

class PlayerExtraData
{
public:
    PlayerExtraData(GamePlayer* pPlayer);
    ~PlayerExtraData();

public:
    bool Set(ePlayerExtraKey key, const Variant& data);

    const Variant& Get(ePlayerExtraKey key) const;
    bool Has(ePlayerExtraKey key) const;

    void Clear(ePlayerExtraKey key);
    void ClearAll();

    void Serialize(MemoryBuffer& memBuffer, bool write);
    uint32 GetMemEstimate() const;

private:
    GamePlayer* m_pPlayer;
    std::vector<Variant> m_data;
};