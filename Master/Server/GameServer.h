#pragma once

#include "Precompiled.h"
#include "Server/ServerBase.h"
#include <unordered_set>

class GameServer : public ServerBase
{
public:
    GameServer();
    ~GameServer();

public:
    static GameServer* GetInstance()
    {
        static GameServer instance;
        return &instance;
    }

public:
    void OnEventConnect(NetworkEvent& event) override;
    void OnEventReceive(NetworkEvent& event) override;
    void OnEventDisconnect(NetworkEvent& event) override;
    void Kill() override;
    void Update() override;

public:
    void LockUserID(uint32 userID);
    void UnLockUserID(uint32 userID);
    bool IsUserLocked(uint32 userID);

private:
    std::unordered_set<uint32> m_lockedUserIDs;
};

GameServer* GetGameServer();