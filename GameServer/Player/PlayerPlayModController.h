#pragma once

#include "Memory/MemoryBuffer.h"
#include "Player/PlayModManager.h"
#include "Player/PlayerPlayMod.h"
#include "Precompiled.h"
#include "Utils/Timer.h"

class DialogBuilder;
class GamePlayer;

class PlayerPlayModController : public PlayerPlayMod
{
public:
    PlayerPlayModController(GamePlayer* pPlayer);
    ~PlayerPlayModController();

public:
    void Update();

    void Serialize(MemoryBuffer& memBuffer, bool write) override;
    void VerifyMods();

    ActivePlayMod* AddPlayMod(ePlayModType type) override;
    bool RemovePlayMod(ePlayModType type) override;
    void RecalculateStats();

    void BuildActiveModsDialog(DialogBuilder& db);

private:
    void OnUpdateTorch(ActivePlayMod& mod);

private:
    GamePlayer* m_pPlayer;
};